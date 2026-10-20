#include "pch.h"
#include "ExportImgui.h"

#define Gecko_Exports_ImGui_GuardNonRender() if (!Gecko::TheScriptManager->IsExecutingRenderCallback()) /* TODO: Throw an error */ (void)0

namespace Gecko::Exports::ImGui
{
	using namespace ::ImGui;

	void BeginAs(std::string& s)
	{
		Gecko_Exports_ImGui_GuardNonRender();
		Begin(s.c_str());
	}

	void EndAs()
	{
		Gecko_Exports_ImGui_GuardNonRender();
		End();
	}

	void TextAs(std::string& s)
	{
		Gecko_Exports_ImGui_GuardNonRender();
		Text(s.c_str());
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