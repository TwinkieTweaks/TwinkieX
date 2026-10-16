#include "pch.h"
#include "Global.h"

namespace Gecko::Exports::Global
{
	void yield()
	{
		auto Main = TheScriptManager->GetActiveScript()->Ctxs.CtxMain;
		if (Main && TheScriptManager->ActiveCallbackType == CallbackType::Main) Main->Suspend();
		else if (TheScriptManager->ActiveCallbackType != CallbackType::Main) (void)0; // TODO: raise an error
	}

	void yieldFor(uint32_t For)
	{
		TheScriptManager->GetActiveScript()->YieldFor = For ? For - 1 : 0;
		if (For) yield();
	}

	void trace(std::u8string& Str)
	{
		LoggertAngelscript.trace(Str);
	}

	void print(std::u8string& Str)
	{
		LoggertAngelscript.print(Str);
	}

	void warn(std::u8string& Str)
	{
		LoggertAngelscript.warn(Str);
	}

	void error(std::u8string& Str)
	{
		LoggertAngelscript.error(Str);
	}

	// TODO: Research if angelscript exception throwing is built-in
	//void _throw(std::u8string& Str)
	//{
	//	LoggertAngelscript.error(Str);
	//}

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterStdString(Engine);
		RegisterScriptArray(Engine, true);

		Engine->RegisterGlobalFunction("void trace(const string &in)", asFUNCTION(trace), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void warn(const string &in)", asFUNCTION(warn), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void error(const string &in)", asFUNCTION(error), asCALL_CDECL);
		// TODO: Research if angelscript exception throwing is built-in
		Engine->RegisterGlobalFunction("void yield()", asFUNCTION(yield), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void yield(uint)", asFUNCTION(yieldFor), asCALL_CDECL);
	}
}