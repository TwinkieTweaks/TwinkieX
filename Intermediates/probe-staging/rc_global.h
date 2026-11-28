#pragma once
#include "pch.h"
#include <Gecko/Awaitable.h>
namespace Gecko
{
	enum class CallbackType : uint32_t { Main = 0, Render };
	struct Script
	{
		std::u8string Name;
		asIScriptContext* Ctxs[2] = {};
		WaitState MainWait;
		std::vector<Coroutine*> Coroutines;
		bool Enabled = true;
		std::string Id = "test";
	};
	struct ScriptManager
	{
		std::vector<Script> Scripts;
		intmax_t ActiveScriptIdx = -1;
		CallbackType ActiveCallbackType = CallbackType::Main;
		bool Running = false;
		Script* GetActiveScript() { return &Scripts[ActiveScriptIdx]; }
		WaitState* CurrentWaitState();
		void StopCoroutines(Script&);
		void RunMainTick(Script&);
		void RunCoroutines(Script&, bool MainTick, RunContext Context);
		void RunContextTick(RunContext);
	};
	extern ScriptManager* TheScriptManager;
	extern bool g_Active[(size_t)RunContext::Count];
	bool IsRunContextActive(RunContext);
	namespace Exports::Global
	{
		WaitState* SuspendCurrentContext();
		void RegisterCoroutines(asIScriptEngine*);
		void RegisterRunContexts(asIScriptEngine*);
	}
}
