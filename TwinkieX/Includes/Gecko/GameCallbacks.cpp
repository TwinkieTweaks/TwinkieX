#include "pch.h"
#include "GameCallbacks.h"
#include "ScriptManager.h"
#include <Twinkie/Twinkie.h>

namespace Gecko
{
#if defined(TMCN) or defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMSX) or defined(TM1)
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

	static bool IsOnScriptThread(const ScriptManager* Manager)
	{
		return Manager and Manager->MainThreadId and Manager->MainThreadId == GetCurrentThreadId();
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

	static void RunContextCallback(void* Param)
	{
		RunContextFired(Param);
	}

	static int32_t LoadGbxHook(void* Archive, CMwNod** Out)
	{
		int32_t Result = s_LoadGbx(Archive, Out);
		NodLoaded(Result, Out);
		return Result;
	}

	static void* CreateRunContextCmd(size_t Idx)
	{
		auto Create = (CmdCreateFn)(gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CMWCMDFASTCALLSTATICPARAM_CREATE);
		void* Cmd = Create(&RunContextCallback, (void*)Idx, RunContextBuffer((RunContext)Idx));
		if (!Cmd) return nullptr;

		ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT)++;
		((CmdMethodFn)Virtual(O_V_CMWCMD_START, Cmd))(Cmd);
		return Cmd;
	}

	static void DestroyRunContextCmd(void* Cmd)
	{
		((CmdMethodFn)Virtual(O_V_CMWCMD_STOP, Cmd))(Cmd);
		((NodReleaseFn)(gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CMWNOD_RELEASE))(Cmd);
	}

	static uintptr_t FindLoadGbx()
	{
		return gTwinkie.TrackmaniaMgr.ExeBaseAddr + O_FN_CSYSTEMARCHIVENOD_LOADGBX;
	}
#elif defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMSX) or defined(TM1)
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
	};

	static uintptr_t SigScanAny(std::initializer_list<const char*> Sigs)
	{
		for (const char* Sig : Sigs)
		{
			if (uintptr_t Found = gTwinkie.TrackmaniaMgr.SigScan(Sig)) return Found;
		}
		return 0;
	}

	static const CmdFunctions& GetCmdFunctions()
	{
		static const CmdFunctions Functions = []
		{
			TwinkTrackmania& Tm = gTwinkie.TrackmaniaMgr;
			CmdFunctions Found;
			Found.FastCallCtor = (FastCallCtorFn)SigScanAny({
				"56 8B F1 E8 ?? ?? ?? ?? 8B 44 24 0C 8B 4C 24 08 89 46 1C C7 06 ?? ?? ?? ?? 89 4E 20 8B C6 5E C2 08 00",
				"0F 57 C0 8B 44 24 08 8B 54 24 04 0F 11 01 C7 01 ?? ?? ?? ?? 83 61 18 FC 89 41 1C 0F 13 41 10 89 51 20 8B C1 C2 08 00",
			});
			Found.SetCoreBuffer = (SetCoreBufferFn)SigScanAny({
				"8B 44 24 04 83 F8 7F 56 8B F1 8B 0D ?? ?? ?? ?? 76 ?? 83 C0 80 83 C1 ?? 50 E8",
				"8B 44 24 04 83 F8 7F 8B 15 ?? ?? ?? ?? 76 ?? 8B 52 ?? 8B 84 82 00 FE FF FF 89 41 14 C2 04 00",
			});
			Found.Start = (CmdMethodFn)Tm.SigScan("56 8B F1 F6 46 18 01 75 ?? 8B 4E 14 56 E8");
			Found.Stop = (CmdMethodFn)Tm.SigScan("8B 41 18 A8 01 74 ?? 83 E0 FE 83 C8 02 6A 00 89 41 18 51 8B 49 14 E8");
			LoggertAngelscript.verbose(LogFormat("Game command functions: FastCall constructor {:#x}, SetCoreBuffer {:#x}, Start {:#x}, Stop {:#x}",
				(uintptr_t)Found.FastCallCtor, (uintptr_t)Found.SetCoreBuffer, (uintptr_t)Found.Start, (uintptr_t)Found.Stop));
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

	static void* CreateRunContextCmd(size_t Idx)
	{
		const CmdFunctions& Functions = GetCmdFunctions();
		if (!Functions.FastCallCtor or !Functions.SetCoreBuffer or !Functions.Start or !Functions.Stop) return nullptr;

		void* Cmd = Functions.FastCallCtor(::operator new(O_SIZE_CMWCMDFASTCALL), (void*)Idx, &RunContextCallback);
		Functions.SetCoreBuffer(Cmd, RunContextBuffer((RunContext)Idx));
		ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT)++;
		Functions.Start(Cmd);
		return Cmd;
	}

	static void DestroyRunContextCmd(void* Cmd)
	{
		GetCmdFunctions().Stop(Cmd);
		if (ReadAddr(int32_t, (uintptr_t)Cmd + O_M_CMWNOD_REFCOUNT) == 1) ::operator delete(Cmd);
	}

	static uintptr_t FindLoadGbx()
	{
		return SigScanAny({
			"56 8B F1 C7 46 08 00 00 00 00 E8 ?? ?? ?? ?? 85 C0 75 06 33 C0 5E C2 04 00",
			"51 53 8B D9 C7 43 08 00 00 00 00 E8 ?? ?? ?? ?? 85 C0 75 05 5B 59 C2 04 00 83 7B 68 03",
			"56 8B F1 57 C7 46 08 00 00 00 00 E8 ?? ?? ?? ?? 85 C0 75 05 5F 5E C2 04 00 83 7E 58 03",
		});
	}
#endif

#if defined(TMCN) or defined(TMU) or defined(ESWC) or defined(TMO) or defined(TMSX) or defined(TM1)
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
		LoggertAngelscript.verbose(u8"Removing the run context commands and the .gbx loader hook");
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

			DestroyRunContextCmd(s_RunContextCmds[Idx]);
			s_RunContextCmds[Idx] = nullptr;
		}
	}

	bool IsRunContextActive(RunContext Context)
	{
		return Context > RunContext::Main and Context < RunContext::Count and s_RunContextActive[(size_t)Context];
	}
#else
	void InstallGameCallbacks()
	{
	}

	void UninstallGameCallbacks()
	{
	}

	bool IsRunContextActive(RunContext)
	{
		return false;
	}
#endif
}
