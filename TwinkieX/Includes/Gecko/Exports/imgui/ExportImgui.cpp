
#include "pch.h"
#include "ExportImgui.h"

#define Gecko_Exports_ImGui_GuardNonRender() if (!Gecko::TheScriptManager->IsExecutingRenderCallback()) /* TODO: Throw an error */ (void)0

namespace Gecko::Exports::ImGui
{
	using namespace ::ImGui;

	// Begin/End functions
	namespace
	{
		/*
		!CompatMeta
		for     UI
		with    bool Begin(const string&in title, int flags = UI::GetDefaultWindowFlags())
		as      bool Begin(const string&in title, int flags)
		being   function
		declare partial
		reason  Flags are not pulled from any function before use.
		!CompatMeta
		*/
		bool BeginAs_NoOpen(std::u8string& title, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			return Begin((const char*)title.c_str(), nullptr, flags);
		}

		/*
		!CompatMeta
		for     UI
		with    bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())
		as      bool Begin(const string&in title, bool&out open, int flags)
		being   function
		declare partial
		reason  Flags are not pulled from any function before use.
		!CompatMeta
		*/
		bool BeginAs_Open(std::u8string& title, bool* open, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			return Begin((const char*)title.c_str(), open, flags);
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
	}

	// Genaral widgets
	namespace
	{
		/*
		!CompatMeta
		for     UI
		with    void Text(const string&in text, int length = -1)
		as      void Text(const string&in text, int length = -1)
		being   function
		declare full
		!CompatMeta
		*/
		void TextAs(std::u8string& s, int length = -1)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			if (length != -1)
			{
				// TODO: Avoid unnecessary copy. Maybe std::u8string_view does the trick?
				std::u8string text(s, 0, length);
				Text((const char*)text.c_str());
			}
			else
			{
				Text((const char*)s.c_str());
			}
		}
	}

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("UI");

		// Begin/End functions
		{
			Engine->RegisterGlobalFunction("bool Begin(const string&in title, int flags)", asFUNCTION(BeginAs_NoOpen), asCALL_CDECL);
			Engine->RegisterGlobalFunction("bool Begin(const string&in title, bool&out open, int flags)", asFUNCTION(BeginAs_Open), asCALL_CDECL);

			Engine->RegisterGlobalFunction("void End()", asFUNCTION(EndAs), asCALL_CDECL);
		}

		// General widgets
		{
			Engine->RegisterGlobalFunction("void Text(const string&in text, int length = -1)", asFUNCTION(TextAs), asCALL_CDECL);
		}

		Engine->SetDefaultNamespace("");
	}
}
