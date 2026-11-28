#include "pch.h"
#include "GameCallbacks.h"
#include "ScriptManager.h"
#include <Twinkie/Twinkie.h>

namespace Gecko
{
	using PeekMessageFn = BOOL(WINAPI*)(LPMSG, HWND, UINT, UINT, UINT);

	static PeekMessageFn s_PeekMessageW = nullptr;
	static PeekMessageFn s_PeekMessageA = nullptr;
	static LPVOID s_PeekMessageWTarget = nullptr;
	static LPVOID s_PeekMessageATarget = nullptr;
	static bool s_SettingsSavedOnExit = false;

	static bool IsOnScriptThread(const ScriptManager* Manager)
	{
		return Manager and Manager->MainThreadId and Manager->MainThreadId == GetCurrentThreadId();
	}

	void SaveSettingsOnExit(const char* Reason)
	{
		ScriptManager* Manager = TheScriptManager;
		if (s_SettingsSavedOnExit or !IsOnScriptThread(Manager)) return;

		s_SettingsSavedOnExit = true;
		LoggertAngelscript.print(LogFormat("Saving the plugin settings before the game exits: {}", Reason));
		Manager->SaveSettings();
	}

	static bool IsQuitMessage(BOOL Result, const MSG* Msg, UINT Remove)
	{
		return Result and Msg and Msg->message == WM_QUIT and (Remove & PM_REMOVE);
	}

	static BOOL WINAPI PeekMessageWHook(LPMSG Msg, HWND Wnd, UINT FilterMin, UINT FilterMax, UINT Remove)
	{
		BOOL Result = s_PeekMessageW(Msg, Wnd, FilterMin, FilterMax, Remove);
		if (IsQuitMessage(Result, Msg, Remove)) SaveSettingsOnExit("the game's message loop got WM_QUIT");
		return Result;
	}

	static BOOL WINAPI PeekMessageAHook(LPMSG Msg, HWND Wnd, UINT FilterMin, UINT FilterMax, UINT Remove)
	{
		BOOL Result = s_PeekMessageA(Msg, Wnd, FilterMin, FilterMax, Remove);
		if (IsQuitMessage(Result, Msg, Remove)) SaveSettingsOnExit("the game's message loop got WM_QUIT");
		return Result;
	}

	static LPVOID HookUser32(const char* Name, LPVOID Detour, LPVOID* Original)
	{
		LPVOID Target = nullptr;
		MH_STATUS Status = MH_CreateHookApiEx(L"user32", Name, Detour, Original, &Target);
		if (Status == MH_OK)
		{
			Status = MH_EnableHook(Target);
			if (Status != MH_OK) MH_RemoveHook(Target);
		}
		if (Status != MH_OK)
		{
			LoggertAngelscript.warn(LogFormat("Couldn't hook {}: {}, so OnSettingsSave won't run when the game quits through it", Name, MH_StatusToString(Status)));
			return nullptr;
		}
		LoggertAngelscript.verbose(LogFormat("Hooked {} at {:#x} to save the plugin settings when the game quits", Name, (uintptr_t)Target));
		return Target;
	}

	static void InstallExitHook()
	{
		if (!s_PeekMessageWTarget) s_PeekMessageWTarget = HookUser32("PeekMessageW", (LPVOID)&PeekMessageWHook, (LPVOID*)&s_PeekMessageW);
		if (!s_PeekMessageATarget) s_PeekMessageATarget = HookUser32("PeekMessageA", (LPVOID)&PeekMessageAHook, (LPVOID*)&s_PeekMessageA);
	}

	static void UninstallExitHook()
	{
		for (LPVOID* Target : { &s_PeekMessageWTarget, &s_PeekMessageATarget })
		{
			if (!*Target) continue;
			MH_DisableHook(*Target);
			MH_RemoveHook(*Target);
			*Target = nullptr;
		}
	}

#if defined(TMCN) or defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMS) or defined(TMSX) or defined(TM1)
	static void* s_RunContextCmds[(size_t)RunContext::Count] = {};
	static bool s_RunContextActive[(size_t)RunContext::Count] = {};
	static bool s_RunContextWarned[(size_t)RunContext::Count] = {};
	static uintptr_t s_LoadGbxTarget = 0;

	static const char* RunContextName(RunContext Context)
	{
		switch (Context)
		{
		case RunContext::Main: return "Main";
		case RunContext::BeforeScripts: return "BeforeScripts";
		case RunContext::MainLoop: return "MainLoop";
		case RunContext::GameLoop: return "GameLoop";
		case RunContext::AfterMainLoop: return "AfterMainLoop";
		case RunContext::NetworkAfterMainLoop: return "NetworkAfterMainLoop";
		case RunContext::AfterScripts: return "AfterScripts";
		case RunContext::UpdateSceneEngine: return "UpdateSceneEngine";
		case RunContext::Count:
		default: return "Unknown";
		}
	}

	static uint32_t RunContextBuffer(RunContext Context)
	{
		switch (Context)
		{
		case RunContext::BeforeScripts: return O_CMDBUFFER_ALLSCRIPTS_BEFORE;
		case RunContext::MainLoop: return O_CMDBUFFER_UPDATE_MAINLOOP;
		case RunContext::GameLoop: return O_CMDBUFFER_UPDATE_GAMEAPPASYNC;
		case RunContext::AfterMainLoop: return O_CMDBUFFER_UPDATE_AFTERMAINLOOP;
		case RunContext::NetworkAfterMainLoop: return O_CMDBUFFER_NETWORK_AFTERMAINLOOP;
		case RunContext::AfterScripts: return O_CMDBUFFER_ALLSCRIPTS_AFTER;
		case RunContext::UpdateSceneEngine: return O_CMDBUFFER_UPDATESCENEENGINE;
		case RunContext::Main:
		case RunContext::Count:
		default: return UINT32_MAX;
		}
	}

	static void RunContextFired(void* Param)
	{
		auto Context = (RunContext)(uintptr_t)Param;
		ScriptManager* Manager = TheScriptManager;
		if (!IsOnScriptThread(Manager))
		{
			if (Manager and !s_RunContextWarned[(size_t)Context])
			{
				s_RunContextWarned[(size_t)Context] = true;
				LoggertAngelscript.warn(LogFormat("Run context {} fired on thread {}, not on the script thread {}, so its coroutines keep running in Main", RunContextName(Context), GetCurrentThreadId(), Manager->MainThreadId));
			}
			return;
		}

		if (!s_RunContextActive[(size_t)Context])
		{
			LoggertAngelscript.verbose(LogFormat("Run context {} fired for the first time", RunContextName(Context)));
			s_RunContextActive[(size_t)Context] = true;
		}
		Manager->RunContextTick(Context);
	}

	static void NodLoaded(int32_t Result, CMwNod** Out)
	{
		ScriptManager* Manager = TheScriptManager;
		if (!Result or !Out or !*Out or !IsOnScriptThread(Manager)) return;

		CMwClassInfo* Info = (*Out)->MwGetClassInfo();
		if (Info) Manager->InvokeLoadCallbacks(*Out, Info->ClassID);
	}
#endif

#ifdef TMCN
	using CmdCallbackFn = void(*)(void*);
	using CmdCreateFn = void*(*)(CmdCallbackFn, void*, uint32_t);
	using CmdMethodFn = void(*)(void*);
	using NodReleaseFn = int32_t(*)(void*);
	using LoadGbxFn = int32_t(*)(void*, CMwNod**);

	static LoadGbxFn s_LoadGbx = nullptr;
	static void* s_SimulationStepCmd = nullptr;

	static void RunContextCallback(void* Param)
	{
		RunContextFired(Param);
	}

	static void SimulationStepCallback(void*)
	{
		ScriptManager* Manager = TheScriptManager;
		if (IsOnScriptThread(Manager)) Manager->RunAll(CallbackType::SimulationStep);
	}

	static int32_t LoadGbxHook(void* Archive, CMwNod** Out)
	{
		int32_t Result = s_LoadGbx(Archive, Out);
		NodLoaded(Result, Out);
		return Result;
	}

	static void* CreateGameCmd(CmdCallbackFn Callback, void* Param, uint32_t Buffer)
	{
		auto Create = (CmdCreateFn)(gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CMWCMDFASTCALLSTATICPARAM_CREATE);
		void* Cmd = Create(Callback, Param, Buffer);
		if (!Cmd) return nullptr;

		ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT)++;
		((CmdMethodFn)Virtual(O_V_CMWCMD_START, Cmd))(Cmd);
		return Cmd;
	}

	static void* CreateRunContextCmd(size_t Idx)
	{
		return CreateGameCmd(&RunContextCallback, (void*)Idx, RunContextBuffer((RunContext)Idx));
	}

	static void DestroyGameCmd(void* Cmd)
	{
		((CmdMethodFn)Virtual(O_V_CMWCMD_STOP, Cmd))(Cmd);
		((NodReleaseFn)(gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CMWNOD_RELEASE))(Cmd);
	}

	static void InstallSimulationStep()
	{
		if (s_SimulationStepCmd) return;

		uintptr_t SimBuffers = ReadAddr(uintptr_t, gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_CORECMD_SIMBUFFERS);
		uintptr_t SimBuffer = SimBuffers ? ReadAddr(uintptr_t, SimBuffers + (O_CMDBUFFER_SIM_HMSCOMPUTENEXTSTATE - O_CMDBUFFER_SIM_FIRST) * sizeof(uintptr_t)) : 0;
		if (!SimBuffer)
		{
			LoggertAngelscript.warn(u8"The game's simulation queues don't exist yet, so SimulationStep won't be called");
			return;
		}

		uint32_t Groups = ReadAddr(uint32_t, SimBuffer + O_M_CMWCMDBUFFER_GROUPCOUNT);
		LoggertAngelscript.verbose(LogFormat("Simulation queue {:#x} is at {:#x} with {} groups", O_CMDBUFFER_SIM_HMSCOMPUTENEXTSTATE, SimBuffer, Groups));
		if (Groups > 1) LoggertAngelscript.warn(LogFormat("The simulation queue has {} groups, so SimulationStep only runs on one simulation step out of {}", Groups, Groups));

		s_SimulationStepCmd = CreateGameCmd(&SimulationStepCallback, nullptr, O_CMDBUFFER_SIM_HMSCOMPUTENEXTSTATE);
		if (s_SimulationStepCmd) LoggertAngelscript.print(LogFormat("SimulationStep runs after HmsComputeNextState, command at {:#x}", (uintptr_t)s_SimulationStepCmd));
		else LoggertAngelscript.warn(u8"Couldn't create the game command for SimulationStep, so it won't be called");
	}

	static void UninstallSimulationStep()
	{
		if (!s_SimulationStepCmd) return;
		DestroyGameCmd(s_SimulationStepCmd);
		s_SimulationStepCmd = nullptr;
	}

	static uintptr_t FindLoadGbx()
	{
		return gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CSYSTEMARCHIVENOD_LOADGBX;
	}
#elif defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMS) or defined(TMSX) or defined(TM1)
	using CmdCallbackFn = void(__fastcall*)(void*);
	using FastCallCtorFn = void*(__thiscall*)(void*, void*, CmdCallbackFn);
	using SetCoreBufferFn = void(__thiscall*)(void*, uint32_t);
	using CmdMethodFn = void(__thiscall*)(void*);
	using LoadGbxFn = int32_t(__thiscall*)(void*, CMwNod**);

	static LoadGbxFn s_LoadGbx = nullptr;

	struct CmdFunctions
	{
		FastCallCtorFn FastCallCtor = nullptr;
		SetCoreBufferFn SetCoreBuffer = nullptr;
		CmdMethodFn Start = nullptr;
		CmdMethodFn Stop = nullptr;
		uintptr_t CoreCmd = 0;
	};

	static uintptr_t FindCoreCmd(uintptr_t SetCoreBuffer)
	{
		return SetCoreBuffer ? ReadAddr(uintptr_t, SetCoreBuffer + O_SIGOFF_CMWCMD_SETCOREBUFFER_CORE) : 0;
	}

	static const CmdFunctions& GetCmdFunctions()
	{
		static const CmdFunctions Functions = []
		{
			TwinkTrackmania& Tm = gTwinkie.TrackmaniaMgr;
			CmdFunctions Found;
			Found.FastCallCtor = (FastCallCtorFn)Tm.SigScan(O_SIG_CMWCMDFASTCALL_CTOR);
			Found.SetCoreBuffer = (SetCoreBufferFn)Tm.SigScan(O_SIG_CMWCMD_SETCOREBUFFER);
			Found.Start = (CmdMethodFn)Tm.SigScan(O_SIG_CMWCMD_START);
			Found.Stop = (CmdMethodFn)Tm.SigScan(O_SIG_CMWCMD_STOP);
			Found.CoreCmd = FindCoreCmd((uintptr_t)Found.SetCoreBuffer);
			LoggertAngelscript.verbose(LogFormat("Game command functions: FastCall constructor {:#x}, SetCoreBuffer {:#x}, Start {:#x}, Stop {:#x}, command core global {:#x}",
				(uintptr_t)Found.FastCallCtor, (uintptr_t)Found.SetCoreBuffer, (uintptr_t)Found.Start, (uintptr_t)Found.Stop, Found.CoreCmd));
			if (!Found.FastCallCtor or !Found.SetCoreBuffer or !Found.Start or !Found.Stop) LoggertAngelscript.warn(u8"Some game command functions weren't found, so run contexts other than Main fall back to Main");
			return Found;
		}();
		return Functions;
	}

	static void __fastcall RunContextCallback(void* Param)
	{
		RunContextFired(Param);
	}

	static int32_t __fastcall LoadGbxHook(void* Archive, void*, CMwNod** Out)
	{
		int32_t Result = s_LoadGbx(Archive, Out);
		NodLoaded(Result, Out);
		return Result;
	}

	static void* CreateGameCmd(CmdCallbackFn Callback, void* Param, uint32_t Buffer)
	{
		const CmdFunctions& Functions = GetCmdFunctions();
		if (!Functions.FastCallCtor or !Functions.SetCoreBuffer or !Functions.Start or !Functions.Stop) return nullptr;

		void* Cmd = Functions.FastCallCtor(::operator new(O_SIZE_CMWCMDFASTCALL), Param, Callback);
		Functions.SetCoreBuffer(Cmd, Buffer);
		ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT)++;
		Functions.Start(Cmd);
		return Cmd;
	}

	static void* CreateRunContextCmd(size_t Idx)
	{
		return CreateGameCmd(&RunContextCallback, (void*)Idx, RunContextBuffer((RunContext)Idx));
	}

	static void DestroyGameCmd(void* Cmd)
	{
		GetCmdFunctions().Stop(Cmd);
		if (ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT) == 1) ::operator delete(Cmd);
	}

#ifdef O_CMDBUFFER_SIM_SIMULATIONSTEP
	static void* s_SimulationStepCmd = nullptr;

	static void __fastcall SimulationStepCallback(void*)
	{
		ScriptManager* Manager = TheScriptManager;
		if (IsOnScriptThread(Manager)) Manager->RunAll(CallbackType::SimulationStep);
	}

	static uintptr_t FindSimBuffer(uint32_t Buffer)
	{
		uintptr_t CoreCmd = GetCmdFunctions().CoreCmd;
		uintptr_t Core = CoreCmd ? ReadAddr(uintptr_t, CoreCmd) : 0;
		if (!Core) return 0;

		uint32_t Idx = Buffer - O_CMDBUFFER_SIM_FIRST;
		uint32_t Count = ReadAddr(uint32_t, Core + O_M_CMWCMDBUFFERCORE_SIMBUFFERCOUNT);
		uintptr_t Buffers = ReadAddr(uintptr_t, Core + O_M_CMWCMDBUFFERCORE_SIMBUFFERS);
		if (Idx >= Count or !Buffers) return 0;
		return ReadAddr(uintptr_t, Buffers + Idx * sizeof(uintptr_t));
	}

	static void InstallSimulationStep()
	{
		if (s_SimulationStepCmd) return;

		uintptr_t SimBuffer = FindSimBuffer(O_CMDBUFFER_SIM_SIMULATIONSTEP);
		if (!SimBuffer)
		{
			LoggertAngelscript.warn(u8"The game's simulation queues don't exist yet, so SimulationStep won't be called");
			return;
		}

		uint32_t Groups = ReadAddr(uint32_t, SimBuffer + O_M_CMWCMDBUFFER_GROUPCOUNT);
		LoggertAngelscript.verbose(LogFormat("Simulation queue {:#x} is at {:#x} with {} groups", O_CMDBUFFER_SIM_SIMULATIONSTEP, SimBuffer, Groups));
		if (Groups > 1) LoggertAngelscript.warn(LogFormat("The simulation queue has {} groups, so SimulationStep only runs on one simulation step out of {}", Groups, Groups));

		s_SimulationStepCmd = CreateGameCmd(&SimulationStepCallback, nullptr, O_CMDBUFFER_SIM_SIMULATIONSTEP);
		if (s_SimulationStepCmd) LoggertAngelscript.print(LogFormat("SimulationStep runs after the dynamic zone step in queue {:#x}, command at {:#x}", O_CMDBUFFER_SIM_HMSZONEDYNAMIC, (uintptr_t)s_SimulationStepCmd));
		else LoggertAngelscript.warn(u8"Couldn't create the game command for SimulationStep, so it won't be called");
	}

	static void UninstallSimulationStep()
	{
		if (!s_SimulationStepCmd) return;
		DestroyGameCmd(s_SimulationStepCmd);
		s_SimulationStepCmd = nullptr;
	}
#else
	static void InstallSimulationStep()
	{
	}

	static void UninstallSimulationStep()
	{
	}
#endif

	static uintptr_t FindLoadGbx()
	{
		return gTwinkie.TrackmaniaMgr.SigScan(O_SIG_CSYSTEMARCHIVENOD_LOADGBX);
	}
#endif

#if defined(TMCN) or defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMS) or defined(TMSX) or defined(TM1)
	void InstallGameCallbacks()
	{
		size_t Installed = 0;
		for (size_t Idx = 1; Idx < (size_t)RunContext::Count; Idx++)
		{
			auto Context = (RunContext)Idx;
			if (!s_RunContextCmds[Idx]) s_RunContextCmds[Idx] = CreateRunContextCmd(Idx);
			if (s_RunContextCmds[Idx])
			{
				Installed++;
				LoggertAngelscript.verbose(LogFormat("Run context {} uses game queue {}, command at {:#x}", RunContextName(Context), RunContextBuffer(Context), (uintptr_t)s_RunContextCmds[Idx]));
			}
			else
			{
				LoggertAngelscript.warn(LogFormat("Couldn't create the game command for run context {}, so its coroutines run in Main", RunContextName(Context)));
			}
		}
		LoggertAngelscript.print(LogFormat("Installed {} of {} run contexts", Installed, (size_t)RunContext::Count - 1));

		InstallSimulationStep();
		InstallExitHook();

		if (s_LoadGbxTarget) return;

		uintptr_t Target = FindLoadGbx();
		if (!Target)
		{
			LoggertAngelscript.warn(u8"Couldn't find the game's .gbx loader, so OnLoadCallback won't be called");
			return;
		}

		MH_STATUS Status = MH_CreateHook((LPVOID)Target, (LPVOID)&LoadGbxHook, (LPVOID*)&s_LoadGbx);
		if (Status == MH_OK)
		{
			Status = MH_EnableHook((LPVOID)Target);
			if (Status != MH_OK) MH_RemoveHook((LPVOID)Target);
		}
		if (Status != MH_OK)
		{
			LoggertAngelscript.warn(LogFormat("Couldn't hook the game's .gbx loader at {:#x}: {}, so OnLoadCallback won't be called", Target, MH_StatusToString(Status)));
			return;
		}

		s_LoadGbxTarget = Target;
		LoggertAngelscript.print(LogFormat("Hooked the game's .gbx loader at {:#x}", Target));
	}

	void UninstallGameCallbacks()
	{
		LoggertAngelscript.verbose(u8"Removing the run context commands, the SimulationStep command and the .gbx loader hook");
		UninstallSimulationStep();
		UninstallExitHook();
		if (s_LoadGbxTarget)
		{
			MH_DisableHook((LPVOID)s_LoadGbxTarget);
			MH_RemoveHook((LPVOID)s_LoadGbxTarget);
			s_LoadGbxTarget = 0;
		}

		for (size_t Idx = 0; Idx < (size_t)RunContext::Count; Idx++)
		{
			s_RunContextActive[Idx] = false;
			if (!s_RunContextCmds[Idx]) continue;

			DestroyGameCmd(s_RunContextCmds[Idx]);
			s_RunContextCmds[Idx] = nullptr;
		}
	}

	bool IsRunContextActive(RunContext Context)
	{
		return Context > RunContext::Main and Context < RunContext::Count and s_RunContextActive[(size_t)Context];
	}
#endif
}
