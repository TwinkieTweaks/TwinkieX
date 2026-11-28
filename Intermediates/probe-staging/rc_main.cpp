#include "pch.h"
#include "Global.h"
#include <fstream>
#include <sstream>
namespace Gecko::Exports::Global { void yield(); void yieldFor(uint32_t); }
using namespace Gecko;

namespace Gecko
{
	ScriptManager* TheScriptManager = nullptr;
	bool g_Active[(size_t)RunContext::Count] = {};
	bool IsRunContextActive(RunContext C) { return C > RunContext::Main && C < RunContext::Count && g_Active[(size_t)C]; }

	static int ExecuteContext(Script& S, asIScriptContext* Ctx)
	{
		int r = Ctx->Execute();
		if (r == asEXECUTION_EXCEPTION) printf("[%s] Exception '%s' line %d\n", S.Id.c_str(), Ctx->GetExceptionString(), Ctx->GetExceptionLineNumber());
		return r;
	}

	WaitState* ScriptManager::CurrentWaitState()
	{
		asIScriptContext* Ctx = asGetActiveContext();
		if (!Ctx || Ctx->IsNested() || ActiveCallbackType != CallbackType::Main) return nullptr;
		if (ActiveScriptIdx < 0 || ActiveScriptIdx >= (intmax_t)Scripts.size()) return nullptr;
		Script& Current = Scripts[ActiveScriptIdx];
		if (Ctx == Current.Ctxs[0]) return &Current.MainWait;
		for (Coroutine* Co : Current.Coroutines) if (Co->Ctx == Ctx) return &Co->Wait;
		return nullptr;
	}

	void ScriptManager::StopCoroutines(Script& S)
	{
		for (Coroutine* Co : S.Coroutines) { Co->Stop(); Co->Release(); }
		S.Coroutines.clear();
		S.MainWait.Clear();
	}

	void ScriptManager::RunMainTick(Script& S)
	{
		asIScriptContext*& Main = S.Ctxs[0];
		if (Main && S.MainWait.Ready())
		{
			if (ExecuteContext(S, Main) != asEXECUTION_SUSPENDED) { Main->Release(); Main = nullptr; S.MainWait.Clear(); }
		}
		RunCoroutines(S, true, RunContext::Main);
	}

	void ScriptManager::RunCoroutines(Script& S, bool MainTick, RunContext Context)
	{
		for (size_t Idx = 0; Idx < S.Coroutines.size() && S.Enabled; Idx++)
		{
			Coroutine* Co = S.Coroutines[Idx];
			bool InContext = MainTick ? !IsRunContextActive(Co->Context) : Co->Context == Context;
			if (!InContext || !Co->Ctx || !Co->Wait.Ready()) continue;
			if (ExecuteContext(S, Co->Ctx) != asEXECUTION_SUSPENDED) Co->Stop();
		}
		std::erase_if(S.Coroutines, [](Coroutine* Co) { if (Co->Running) return false; Co->Release(); return true; });
	}

	void ScriptManager::RunContextTick(RunContext Context)
	{
		if (Running) return;
		for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
		{
			Script& S = Scripts[Idx];
			if (!S.Enabled || S.Coroutines.empty()) continue;
			Running = true; ActiveScriptIdx = (intmax_t)Idx; ActiveCallbackType = CallbackType::Main;
			RunCoroutines(S, false, Context);
			ActiveScriptIdx = -1; Running = false;
		}
	}
}

static int Tick = 0;
static const char* Stage = "";
static void Msg(const asSMessageInfo* m, void*) { printf("%s (%d,%d): %s\n", m->section, m->row, m->col, m->message); }
static void Print(const std::string& s) { printf("[t%d %s] %s\n", Tick, Stage, s.c_str()); }

int main(int argc, char** argv)
{
	asIScriptEngine* E = asCreateScriptEngine();
	E->SetMessageCallback(asFUNCTION(Msg), 0, asCALL_CDECL);
	RegisterStdString(E);
	RegisterScriptArray(E, true);
	Exports::Global::RegisterCoroutines(E);
	E->RegisterGlobalFunction("void yield()", asFUNCTION(Exports::Global::yield), asCALL_CDECL);
	E->RegisterGlobalFunction("void print(const string&in)", asFUNCTION(Print), asCALL_CDECL);
	E->SetDefaultNamespace("Meta");
	Exports::Global::RegisterRunContexts(E);
	E->SetDefaultNamespace("");

	bool Hooked = argc > 2 && atoi(argv[2]) != 0;
	if (Hooked) for (size_t i = 1; i < (size_t)RunContext::Count; i++) g_Active[i] = true;

	std::ifstream F(argv[1]); std::stringstream SS; SS << F.rdbuf();
	asIScriptModule* M = E->GetModule("m", asGM_ALWAYS_CREATE);
	M->AddScriptSection("t.as", SS.str().c_str());
	if (M->Build() < 0) { printf("BUILD FAILED\n"); return 1; }

	ScriptManager SM; TheScriptManager = &SM;
	SM.Scripts.emplace_back();
	Script& S = SM.Scripts[0];
	S.Name = u8"m";
	S.Ctxs[0] = E->CreateContext(); S.Ctxs[0]->Prepare(M->GetFunctionByDecl("void Main()"));

	const char* Names[] = { "Main", "BeforeScripts", "MainLoop", "GameLoop", "AfterMainLoop", "NetworkAfterMainLoop", "AfterScripts", "UpdateSceneEngine" };
	for (Tick = 0; Tick < 4; Tick++)
	{
		for (size_t c = 1; c < (size_t)RunContext::Count; c++)
		{
			if (!g_Active[c]) continue;
			Stage = Names[c];
			SM.RunContextTick((RunContext)c);
		}
		Stage = "Main";
		SM.ActiveScriptIdx = 0; SM.ActiveCallbackType = CallbackType::Main; SM.Running = true;
		SM.RunMainTick(S);
		SM.ActiveScriptIdx = -1; SM.Running = false;
	}
	printf("Coroutines left: %zu\n", S.Coroutines.size());
	SM.StopCoroutines(S);
	if (S.Ctxs[0]) S.Ctxs[0]->Release();
	M->Discard();
	E->ShutDownAndRelease();
	printf("Shutdown OK\n");
}
