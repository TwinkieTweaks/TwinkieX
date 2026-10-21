
#include "pch.h"
#include "ExportImgui.h"

#define Gecko_Exports_ImGui_GuardNonRender() if (!Gecko::TheScriptManager->IsExecutingRenderCallback()) /* TODO: Throw an error */ (void)0

namespace Gecko::Exports::ImGui
{
	using namespace ::ImGui;

	/*
	!CompatMeta
	for     UI
	with    bool Begin(const string&in title, int flags = UI::GetDefaultWindowFlags())
	with    bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())
	as      void Begin(const string&in title)
	being   function
	declare partial
	reason  Open and Flags are not implemented as arguments yet.
	!CompatMeta
	*/
	void BeginAs(std::u8string& s)
	{
		Gecko_Exports_ImGui_GuardNonRender();
		Begin((const char*)s.c_str());
	}

	/*
	!CompatMeta
	for     UI
	with    void End()
	as      void End()
	being   function
	declare full
	!CompatMeta
	*/
	void EndAs()
	{
		Gecko_Exports_ImGui_GuardNonRender();
		End();
	}

	/*
	!CompatMeta
	for     UI
	with    void Text(const string&in text, int length = -1)
	as      void Text(const string&in text)
	being   function
	declare partial
	reason  Length is not implemented.
	!CompatMeta
	*/
	void TextAs(std::u8string& s)
	{
		Gecko_Exports_ImGui_GuardNonRender();
		Text((const char*)s.c_str());
	}

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("UI");

		Engine->RegisterGlobalFunction("void Begin(string&in)", asFUNCTION(BeginAs), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void Text(string&in)", asFUNCTION(TextAs), asCALL_CDECL);
		Engine->RegisterGlobalFunction("void End()", asFUNCTION(EndAs), asCALL_CDECL);

		Engine->SetDefaultNamespace("");
	}
}
