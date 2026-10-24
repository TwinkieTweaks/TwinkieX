
#include "pch.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include "ExportImgui.h"

#define Gecko_Exports_ImGui_GuardNonRender() if (!Gecko::TheScriptManager->IsExecutingRenderCallback()) /* TODO: Throw an error */ (void)0

namespace Gecko::Exports::ImGui
{
	using namespace ::ImGui;

	static std::vector<GeckoExport> s_Exports;
#define UI_EXPORT(Decl, Fn) GECKO_EXPORT_FN(s_Exports, "UI", Decl, Fn)

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
		since   v0.1.0
		reason  Flags are not pulled from any function before use.
		!CompatMeta
		*/
		bool BeginAs_NoOpen(std::u8string& title, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			return Begin((const char*)title.c_str(), nullptr, flags);
		}
		UI_EXPORT("bool Begin(const string&in title, int flags = 0)", BeginAs_NoOpen);

		/*
		!CompatMeta
		for     UI
		with    bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())
		as      bool Begin(const string&in title, bool&out open, int flags)
		being   function
		declare partial
		since   v0.1.0
		reason  Flags are not pulled from any function before use.
		!CompatMeta
		*/
		bool BeginAs_Open(std::u8string& title, bool* open, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			return Begin((const char*)title.c_str(), open, flags);
		}
		UI_EXPORT("bool Begin(const string&in title, bool&out open, int flags = 0)", BeginAs_Open);

		/*
		!CompatMeta
		for     UI
		with    void End()
		as      void End()
		being   function
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		void EndAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			End();
		}
		UI_EXPORT("void End()", EndAs);

		/*
		!CompatMeta
		for     UI
		with    bool BeginCombo(const string&in label, const string&in current, int flags = UI::ComboFlags::None)
		as      bool BeginCombo(const string&in label, const string&in current, int flags)
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		bool BeginComboAs(std::u8string& label, std::u8string& current, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			return BeginCombo((const char*)label.c_str(), (const char*)current.c_str(), flags);
		}
		UI_EXPORT("bool BeginCombo(const string&in label, const string&in current, int flags = 0)", BeginComboAs);

		/*
		!CompatMeta
		for     UI
		with    void EndCombo()
		as      void EndCombo()
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		void EndComboAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			EndCombo();
		}
		UI_EXPORT("void EndCombo()", EndComboAs);

		/*
		!CompatMeta
		for     UI
		with    void BeginDisabled(bool disabled = true)
		as      void BeginDisabled(bool disabled = true)
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		void BeginDisabledAs(bool disabled)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			BeginDisabled(disabled);
		}
		UI_EXPORT("void BeginDisabled(bool disabled = true)", BeginDisabledAs);

		/*
		!CompatMeta
		for     UI
		with    void EndDisabled()
		as      void EndDisabled()
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		void EndDisabledAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			EndDisabled();
		}
		UI_EXPORT("void EndDisabled()", EndDisabledAs);

		/*
		!CompatMeta
		for     UI
		with    void BeginTabBar(const string&in id, int flags = UI::TabBarFlags::None)
		as      void BeginTabBar(const string&in id, int flags)
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		void BeginTabBarAs(std::u8string& id, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			BeginTabBar((const char*)id.c_str(), flags);
		}
		UI_EXPORT("void BeginTabBar(const string&in id, int flags = 0)", BeginTabBarAs);

		/*
		!CompatMeta
		for     UI
		with    void EndTabBar()
		as      void EndTabBar()
		being   function
		declare full
		since   v0.1.1
		!CompatMeta
		*/
		void EndTabBarAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			EndTabBar();
		}
		UI_EXPORT("void EndTabBar()", EndTabBarAs);
	}

	// General widgets
	namespace
	{
		/*
		!CompatMeta
		for     UI
		with    void Text(const string&in text, int length = -1)
		as      void Text(const string&in text, int length = -1)
		being   function
		declare full
		since   v0.1.0
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
		UI_EXPORT("void Text(const string&in text, int length = -1)", TextAs);
	}

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterAll(Engine, s_Exports);
	}
}
