
#include "pch.h"
#include "Global.h"
#include <Gecko/Exports/GeckoAutoExport.h>

namespace Gecko::Exports::Global
{
	static std::vector<GeckoExport> s_Exports;
#define GLOBAL_EXPORT(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "", Decl_, Fn_)

	/*
	!CompatMeta
	for     Global
	with    void yield()
	as      void yield()
	being   function
	declare full
	!CompatMeta
	*/
	void yield()
	{
		auto Main = TheScriptManager->GetActiveScript()->Ctxs.CtxMain;
		if (Main && TheScriptManager->ActiveCallbackType == CallbackType::Main) Main->Suspend();
		else if (TheScriptManager->ActiveCallbackType != CallbackType::Main) (void)0; // TODO: raise an error
	}
	GLOBAL_EXPORT("void yield()", yield);

	/*
	!CompatMeta
	for     Global
	with    void yield(uint frames)
	as      void yield(uint frames)
	being   function
	declare full
	!CompatMeta
	*/
	void yieldFor(uint32_t For)
	{
		TheScriptManager->GetActiveScript()->YieldFor = For ? For - 1 : 0;
		if (For) yield();
	}
	GLOBAL_EXPORT("void yield(uint)", yieldFor);

	/*
	!CompatMeta
	for     Global
	with    void trace(const string&in)
	as      void trace(const string&in)
	being   function
	declare partial
	reason  Did not implement generic yet (where first arg is anything)
	!CompatMeta
	*/
	void trace(std::u8string& Str)
	{
		LoggertAngelscript.trace(Str);
	}
	GLOBAL_EXPORT("void trace(const string &in)", trace);

	/*
	!CompatMeta
	for     Global
	with    void print(const string&in)
	as      void print(const string&in)
	being   function
	declare partial
	reason  Did not implement generic yet (where first arg is anything)
	!CompatMeta
	*/
	void print(std::u8string& Str)
	{
		LoggertAngelscript.print(Str);
	}
	GLOBAL_EXPORT("void print(const string &in)", print);

	/*
	!CompatMeta
	for     Global
	with    void warn(const string&in)
	as      void warn(const string&in)
	being   function
	declare partial
	reason  Did not implement generic yet (where first arg is anything)
	!CompatMeta
	*/
	void warn(std::u8string& Str)
	{
		LoggertAngelscript.warn(Str);
	}
	GLOBAL_EXPORT("void warn(const string &in)", warn);

	/*
	!CompatMeta
	for     Global
	with    void error(const string&in)
	as      void error(const string&in)
	being   function
	declare partial
	reason  Did not implement generic yet (where first arg is anything)
	!CompatMeta
	*/
	void error(std::u8string& Str)
	{
		LoggertAngelscript.error(Str);
	}
	GLOBAL_EXPORT("void error(const string &in)", error);

	// TODO: Research if angelscript exception throwing is built-in
	//void _throw(std::u8string& Str)
	//{
	//	LoggertAngelscript.error(Str);
	//}

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterStdString(Engine);
		RegisterScriptArray(Engine, true);

		RegisterAll(Engine, s_Exports);
	}
}
