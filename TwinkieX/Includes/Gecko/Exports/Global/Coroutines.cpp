#include "pch.h"
#include "Global.h"
#include <Gecko/Awaitable.h>
#include <angelscript/sdk/add_on/scripthandle/scripthandle.h>

namespace Gecko::Exports::Global
{
	static void Throw(const char* Message)
	{
		if (auto Ctx = asGetActiveContext()) Ctx->SetException(Message);
	}

	static WaitState* YieldableWait()
	{
		WaitState* Wait = TheScriptManager->CurrentWaitState();
		if (!Wait) Throw("Can't yield from a non-yieldable context");
		return Wait;
	}

	WaitState* SuspendCurrentContext()
	{
		WaitState* Wait = YieldableWait();
		if (Wait) asGetActiveContext()->Suspend();
		return Wait;
	}

	static bool Awaitable_IsRunning(Awaitable* Self)
	{
		return Self->IsRunning();
	}

	static bool IsValidRunContext(int32_t Context)
	{
		return Context >= 0 and Context < (int32_t)RunContext::Count;
	}

	static Awaitable* Awaitable_WithRunContext(int Context, Awaitable* Self)
	{
		if (!IsValidRunContext(Context))
		{
			Throw("Invalid run context");
			return nullptr;
		}
		Self->SetRunContext((RunContext)Context);
		Self->AddRef();
		return Self;
	}

	static void Awaitable_AddRef(Awaitable* Self)
	{
		Self->AddRef();
	}

	static void Awaitable_Release(Awaitable* Self)
	{
		Self->Release();
	}

	static void SleepFor(uint64_t Ms)
	{
		WaitState* Wait = SuspendCurrentContext();
		if (Wait && Ms) Wait->WakeAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(Ms);
	}

	static void AwaitOne(Awaitable* Item)
	{
		WaitState* Wait = YieldableWait();
		if (!Wait || !Item || !Item->IsRunning()) return;

		Item->AddRef();
		Wait->Awaiting.push_back(Item);
		Wait->AwaitAny = false;
		asGetActiveContext()->Suspend();
	}

	static void AwaitItems(CScriptArray* Items, bool Any)
	{
		WaitState* Wait = YieldableWait();
		if (!Wait || !Items) return;

		std::vector<Awaitable*> Running;
		bool SomeDone = false;
		for (asUINT Idx = 0; Idx < Items->GetSize(); Idx++)
		{
			Awaitable* Item = *(Awaitable**)Items->At(Idx);
			if (Item && Item->IsRunning()) Running.push_back(Item);
			else SomeDone = true;
		}
		if (Running.empty() || (Any && SomeDone)) return;

		for (Awaitable* Item : Running)
		{
			Item->AddRef();
			Wait->Awaiting.push_back(Item);
		}
		Wait->AwaitAny = Any;
		asGetActiveContext()->Suspend();
	}

	static void AwaitAll(CScriptArray* Items)
	{
		AwaitItems(Items, false);
	}

	static void AwaitAny(CScriptArray* Items)
	{
		AwaitItems(Items, true);
	}

	static Script* ScriptOfFunction(asIScriptFunction* Func)
	{
		asIScriptFunction* Real = Func->GetDelegateFunction() ? Func->GetDelegateFunction() : Func;
		if (asIScriptModule* Module = Real->GetModule())
		{
			for (auto& Candidate : TheScriptManager->Scripts)
			{
				if (Candidate.Name == (const char8_t*)Module->GetName()) return &Candidate;
			}
		}
		return TheScriptManager->ActiveScriptIdx >= 0 ? TheScriptManager->GetActiveScript() : nullptr;
	}

	static Coroutine* CreateCoroutine(asIScriptGeneric* Gen, asUINT FuncArg)
	{
		RunContext Context = RunContext::Main;
		if (FuncArg > 0)
		{
			auto Requested = (int32_t)Gen->GetArgDWord(0);
			if (!IsValidRunContext(Requested))
			{
				Throw("Invalid run context");
				return nullptr;
			}
			Context = (RunContext)Requested;
		}

		auto Func = (asIScriptFunction*)Gen->GetArgAddress(FuncArg);
		if (!Func)
		{
			Throw("Null pointer access");
			return nullptr;
		}

		Script* Owner = ScriptOfFunction(Func);
		if (!Owner)
		{
			Throw("Can't start a coroutine outside of a script");
			return nullptr;
		}

		Coroutine* Co = new Coroutine();
		Co->Ctx = Gen->GetEngine()->CreateContext();
		if (!Co->Ctx || Co->Ctx->Prepare(Func) < 0)
		{
			Co->Release();
			Throw("Couldn't start the coroutine");
			return nullptr;
		}

		Co->Context = Context;
		Owner->Coroutines.push_back(Co);
		return Co;
	}

	template<asUINT FuncArg>
	static void StartNew(asIScriptGeneric* Gen)
	{
		Gen->SetReturnObject(CreateCoroutine(Gen, FuncArg));
	}

	template<asUINT FuncArg>
	static void StartNewRef(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgObject(0, Gen->GetArgObject(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewInt(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgQWord(0, (asQWORD)(int64_t)(int32_t)Gen->GetArgDWord(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewUint(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgQWord(0, (asQWORD)Gen->GetArgDWord(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewQWord(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgQWord(0, Gen->GetArgQWord(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewDouble(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgDouble(0, Gen->GetArgDouble(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewBool(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co) Co->Ctx->SetArgByte(0, Gen->GetArgByte(FuncArg + 1));
		Gen->SetReturnObject(Co);
	}

	template<asUINT FuncArg>
	static void StartNewString(asIScriptGeneric* Gen)
	{
		Coroutine* Co = CreateCoroutine(Gen, FuncArg);
		if (Co)
		{
			Co->StringArg = *(const std::string*)Gen->GetArgObject(FuncArg + 1);
			Co->Ctx->SetArgAddress(0, &Co->StringArg);
		}
		Gen->SetReturnObject(Co);
	}

	void RegisterRunContexts(asIScriptEngine* Engine)
	{
		/*
		!CompatMeta
		for     Meta
		with    RunContext
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterEnum("RunContext");
		Engine->RegisterEnumValue("RunContext", "Main", (int)RunContext::Main);
		Engine->RegisterEnumValue("RunContext", "BeforeScripts", (int)RunContext::BeforeScripts);
		Engine->RegisterEnumValue("RunContext", "MainLoop", (int)RunContext::MainLoop);
		Engine->RegisterEnumValue("RunContext", "GameLoop", (int)RunContext::GameLoop);
		Engine->RegisterEnumValue("RunContext", "AfterMainLoop", (int)RunContext::AfterMainLoop);
		Engine->RegisterEnumValue("RunContext", "NetworkAfterMainLoop", (int)RunContext::NetworkAfterMainLoop);
		Engine->RegisterEnumValue("RunContext", "AfterScripts", (int)RunContext::AfterScripts);
		Engine->RegisterEnumValue("RunContext", "UpdateSceneEngine", (int)RunContext::UpdateSceneEngine);

		/*
		!CompatMeta
		for     Meta
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFunc@ func)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdata@ func, ref userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@ func, const int userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@ func, const int64 userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@ func, const uint userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@ func, const uint64 userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataDouble@ func, const double userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataString@ func, const string&in userdata)
		being   function
		declare full
		games   TMCN, TMU, ESWC, TMO, TMSX, TM1
		since   v0.2.0
		!CompatMeta
		*/
		/*
		!CompatMeta
		for     Meta
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFunc@ func)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdata@ func, ref userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@ func, const int userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@ func, const int64 userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@ func, const uint userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@ func, const uint64 userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataDouble@ func, const double userdata)
		with    awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataString@ func, const string&in userdata)
		being   function
		declare partial
		reason  Run contexts other than Main aren't hooked yet, so those coroutines run in the main loop
		games   TMS
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFunc@+ func)", asFUNCTION(StartNew<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdata@+ func, ref userdata)", asFUNCTION(StartNewRef<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@+ func, const int userdata)", asFUNCTION(StartNewInt<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataInt64@+ func, const int64 userdata)", asFUNCTION(StartNewQWord<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@+ func, const uint userdata)", asFUNCTION(StartNewUint<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataUint64@+ func, const uint64 userdata)", asFUNCTION(StartNewQWord<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataDouble@+ func, const double userdata)", asFUNCTION(StartNewDouble<1>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ StartWithRunContext(RunContext runContext, CoroutineFuncUserdataString@+ func, const string&in userdata)", asFUNCTION(StartNewString<1>), asCALL_GENERIC);
	}

	void RegisterCoroutines(asIScriptEngine* Engine)
	{
		/*
		!CompatMeta
		for     Global
		with    ~ref()
		with    void $beh12(int&in)
		with    void $beh13(int&in)
		with    ref()
		with    ref(const ref&in)
		with    ref(const ?&in)
		being   behavior
		of      ref
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		/*
		!CompatMeta
		for     Global
		with    void opCast(?&out)
		with    ref& opHndlAssign(const ref&in)
		with    ref& opHndlAssign(const ?&in)
		with    bool opEquals(const ref&in) const
		with    bool opEquals(const ?&in) const
		being   method
		of      ref
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterScriptHandle(Engine);

		/*
		!CompatMeta
		for     Global
		with    void $beh5()
		with    void $beh6()
		being   behavior
		of      awaitable
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectType("awaitable", 0, asOBJ_REF);
		Engine->RegisterObjectBehaviour("awaitable", asBEHAVE_ADDREF, "void f()", asFUNCTION(Awaitable_AddRef), asCALL_CDECL_OBJLAST);
		Engine->RegisterObjectBehaviour("awaitable", asBEHAVE_RELEASE, "void f()", asFUNCTION(Awaitable_Release), asCALL_CDECL_OBJLAST);

		/*
		!CompatMeta
		for     Global
		with    bool IsRunning()
		as      bool IsRunning()
		being   method
		of      awaitable
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectMethod("awaitable", "bool IsRunning()", asFUNCTION(Awaitable_IsRunning), asCALL_CDECL_OBJLAST);

		/*
		!CompatMeta
		for     Global
		with    awaitable@ WithRunContext(int)
		as      awaitable@ WithRunContext(int)
		being   method
		of      awaitable
		declare full
		games   TMCN, TMU, ESWC, TMO, TMSX, TM1
		since   v0.2.0
		!CompatMeta
		*/
		/*
		!CompatMeta
		for     Global
		with    awaitable@ WithRunContext(int)
		as      awaitable@ WithRunContext(int)
		being   method
		of      awaitable
		declare partial
		reason  Run contexts other than Main aren't hooked yet, so the coroutine keeps running in the main loop
		games   TMS
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectMethod("awaitable", "awaitable@ WithRunContext(int)", asFUNCTION(Awaitable_WithRunContext), asCALL_CDECL_OBJLAST);

		/*
		!CompatMeta
		for     Global
		with    void CoroutineFunc()
		with    void CoroutineFuncUserdata(ref)
		with    void CoroutineFuncUserdataInt64(int64)
		with    void CoroutineFuncUserdataUint64(uint64)
		with    void CoroutineFuncUserdataDouble(double)
		with    void CoroutineFuncUserdataBool(bool)
		with    void CoroutineFuncUserdataString(const string&in)
		being   funcdef
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterFuncdef("void CoroutineFunc()");
		Engine->RegisterFuncdef("void CoroutineFuncUserdata(ref)");
		Engine->RegisterFuncdef("void CoroutineFuncUserdataInt64(int64)");
		Engine->RegisterFuncdef("void CoroutineFuncUserdataUint64(uint64)");
		Engine->RegisterFuncdef("void CoroutineFuncUserdataDouble(double)");
		Engine->RegisterFuncdef("void CoroutineFuncUserdataBool(bool)");
		Engine->RegisterFuncdef("void CoroutineFuncUserdataString(const string&in)");

		/*
		!CompatMeta
		for     Global
		with    awaitable@ startnew(CoroutineFunc@ func)
		with    awaitable@ startnew(CoroutineFuncUserdata@ func, ref userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataInt64@ func, const int userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataInt64@ func, const int64 userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataUint64@ func, const uint userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataUint64@ func, const uint64 userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataDouble@ func, const double userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataBool@ func, const bool userdata)
		with    awaitable@ startnew(CoroutineFuncUserdataString@ func, const string&in userdata)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFunc@+ func)", asFUNCTION(StartNew<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdata@+ func, ref userdata)", asFUNCTION(StartNewRef<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataInt64@+ func, const int userdata)", asFUNCTION(StartNewInt<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataInt64@+ func, const int64 userdata)", asFUNCTION(StartNewQWord<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataUint64@+ func, const uint userdata)", asFUNCTION(StartNewUint<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataUint64@+ func, const uint64 userdata)", asFUNCTION(StartNewQWord<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataDouble@+ func, const double userdata)", asFUNCTION(StartNewDouble<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataBool@+ func, const bool userdata)", asFUNCTION(StartNewBool<0>), asCALL_GENERIC);
		Engine->RegisterGlobalFunction("awaitable@ startnew(CoroutineFuncUserdataString@+ func, const string&in userdata)", asFUNCTION(StartNewString<0>), asCALL_GENERIC);

		/*
		!CompatMeta
		for     Global
		with    void await(awaitable@)
		with    void await(awaitable@[]@)
		with    void awaitany(awaitable@[]@)
		with    void sleep(uint64 ms)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterGlobalFunction("void await(awaitable@+)", asFUNCTION(AwaitOne), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void await(awaitable@[]@+)", asFUNCTION(AwaitAll), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void awaitany(awaitable@[]@+)", asFUNCTION(AwaitAny), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void sleep(uint64 ms)", asFUNCTION(SleepFor), asCALL_CDECL);
	}
}
