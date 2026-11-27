
#include "pch.h"
#include <unordered_map>
#include <Gecko/Exports/GeckoAutoExport.h>
#include "ExportImgui.h"
#include "Texture/Texture.h"
#include "Font/Font.h"
#include <Gecko/Exports/Global/MemoryBuffer/MemoryBuffer.h>
#include <Gecko/Exports/Math/Math.h>
#include <Twinkie/TwinkUi/TwinkUi.h>

#define Gecko_Exports_ImGui_GuardNonRender(...) if (!Gecko::TheScriptManager->IsExecutingRenderCallback()) /* TODO: Throw an error */ { (void)0; return __VA_ARGS__; } (void)0

namespace Gecko::Exports::ImGui
{
	using namespace ::ImGui;

	static std::vector<GeckoExport> s_Exports;
#define UI_EXPORT(Decl, Fn) GECKO_EXPORT_FN(s_Exports, "UI", Decl, Fn)
#define Gecko_Exports_ImGui_MakeEndFn(Fn) void End##Fn##As() { \
			Gecko_Exports_ImGui_GuardNonRender(); \
			End##Fn(); \
		} UI_EXPORT("void End" #Fn "()", End##Fn##As)
#define Gecko_Exports_ImGui_MakeTrivialFn(Fn) void Fn##As() { \
			Gecko_Exports_ImGui_GuardNonRender(); \
			Fn(); \
		} UI_EXPORT("void " #Fn "()", Fn##As)
#define Gecko_Exports_ImGui_MakeTrivialReturnFn(Fn, ReturnType) decltype(Fn()) Fn##As() { \
			Gecko_Exports_ImGui_GuardNonRender(decltype(Fn()){}); \
			return Fn(); \
		} UI_EXPORT(#ReturnType " " #Fn "()", Fn##As)

	// UI::Texture
	namespace
	{
		/*
		!CompatMeta
		for     UI
		with    Texture@ LoadTexture(const string&in filename)
		as      Texture@ LoadTexture(const string&in filename)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Texture* LoadTextureAs_File(std::u8string& FileName)
		{
			Texture* Tex = new Texture();
			if (!Tex->From(std::filesystem::path(FileName)))
			{
				delete Tex;
				return nullptr;
			}
			return Tex;
		}
		UI_EXPORT("Texture@+ LoadTexture(const string&in filename)", LoadTextureAs_File);

		/*
		!CompatMeta
		for     UI
		with    Texture@ LoadTexture(MemoryBuffer&in buffer)
		as      Texture@ LoadTexture(MemoryBuffer&in buffer)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Texture* LoadTextureAs_Buffer(MemoryBuffer& Buffer)
		{
			if (!Buffer.GetSize()) return nullptr;

			uint64_t Position = Buffer.GetPosition();
			Buffer.Seek(0);
			std::u8string Bytes = Buffer.ReadString(Buffer.GetSize());
			Buffer.Seek(Position);

			Texture* Tex = new Texture();
			if (!Tex->From(Bytes.data(), Bytes.size()))
			{
				delete Tex;
				return nullptr;
			}
			return Tex;
		}
		UI_EXPORT("Texture@+ LoadTexture(MemoryBuffer&in buffer)", LoadTextureAs_Buffer);
	}

	// UI::Font
	namespace
	{
		// Glyph ranges from a script: pairs of first and last characters, optionally ending with 0
		std::vector<ImWchar> RangesFrom(const CScriptArray* Ranges)
		{
			std::vector<ImWchar> Out;
			if (!Ranges) return Out;

			for (asUINT i = 0; i < Ranges->GetSize(); i++)
			{
				uint32_t Char = *(const uint32_t*)Ranges->At(i);
				if (Char == 0) break;
				Out.push_back((ImWchar)(std::min)(Char, (uint32_t)IM_UNICODE_CODEPOINT_MAX));
			}
			if (Out.size() % 2) Out.pop_back();
			return Out;
		}

		std::string ToStr(const std::u8string& Str)
		{
			return std::string(Str.begin(), Str.end());
		}

		/*
		!CompatMeta
		for     UI
		with    float get_FontSize()
		as      float get_FontSize() const property
		being   method
		of      Font
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		float FontGetFontSize(const ScriptFont* Font)
		{
			return Font->Size;
		}
		GECKO_EXPORT_METHOD(s_Exports, "UI", "Font", "float get_FontSize() const property", asFUNCTION(FontGetFontSize), asCALL_CDECL_OBJLAST);

		/*
		!CompatMeta
		for     UI::Font
		with    UI::Font@ get_Default()
		with    UI::Font@ get_DefaultBold()
		with    UI::Font@ get_DefaultMono()
		as      UI::Font@ get_Default() property
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		GECKO_EXPORT_FN(s_Exports, "UI::Font", "UI::Font@+ get_Default() property", ScriptFont::GetDefault);
		GECKO_EXPORT_FN(s_Exports, "UI::Font", "UI::Font@+ get_DefaultBold() property", ScriptFont::GetDefaultBold);
		GECKO_EXPORT_FN(s_Exports, "UI::Font", "UI::Font@+ get_DefaultMono() property", ScriptFont::GetDefaultMono);

		/*
		!CompatMeta
		for     UI
		with    Font@ LoadFont(const string&in filename, float size = 16.0f)
		as      Font@ LoadFont(const string&in filename, float size = 16.0f)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		ScriptFont* LoadFontAs(std::u8string& Filename, float Size)
		{
			return ScriptFont::Load(ToStr(Filename), Size);
		}
		UI_EXPORT("Font@+ LoadFont(const string&in filename, float size = 16.0f)", LoadFontAs);

		/*
		!CompatMeta
		for     UI
		with    Font@ LoadFont(const string&in filename, float size, const uint[]@, bool ba = false, bool bb = false, bool bc = false)
		as      Font@ LoadFont(const string&in filename, float size, const uint[]@ ranges, bool ba = false, bool bb = false, bool bc = false)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		ScriptFont* LoadFontAs_Ranges(std::u8string& Filename, float Size, CScriptArray* Ranges, bool, bool, bool)
		{
			return ScriptFont::Load(ToStr(Filename), Size, RangesFrom(Ranges));
		}
		UI_EXPORT("Font@+ LoadFont(const string&in filename, float size, const uint[]@+ ranges, bool ba = false, bool bb = false, bool bc = false)", LoadFontAs_Ranges);

		/*
		!CompatMeta
		for     UI
		with    Font@ LoadFont(const string&in filename, float size, int minChars, int maxChars, bool = false, bool = false, bool = false)
		as      Font@ LoadFont(const string&in filename, float size, int minChars, int maxChars, bool = false, bool = false, bool = false)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		ScriptFont* LoadFontAs_MinMax(std::u8string& Filename, float Size, int MinChars, int MaxChars, bool, bool, bool)
		{
			auto Clamp = [](int Char) { return (ImWchar)(std::max)(1, (std::min)(Char, (int)IM_UNICODE_CODEPOINT_MAX)); };
			return ScriptFont::Load(ToStr(Filename), Size, { Clamp(MinChars), Clamp(MaxChars) });
		}
		UI_EXPORT("Font@+ LoadFont(const string&in filename, float size, int minChars, int maxChars, bool = false, bool = false, bool = false)", LoadFontAs_MinMax);

		/*
		!CompatMeta
		for     UI
		with    Font@ LoadSystemFont(const string&in filename)
		as      Font@ LoadSystemFont(const string&in filename)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		ScriptFont* LoadSystemFontAs(std::u8string& Filename)
		{
			return ScriptFont::LoadSystem(ToStr(Filename));
		}
		UI_EXPORT("Font@+ LoadSystemFont(const string&in filename)", LoadSystemFontAs);

		/*
		!CompatMeta
		for     UI
		with    void PushFont(Font@ font)
		as      void PushFont(Font@ font)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PushFontAs(ScriptFont* Font)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushFont(Font ? Font->Get() : nullptr, Font ? Font->Size : 0.0f);
		}
		UI_EXPORT("void PushFont(Font@+ font)", PushFontAs);

		/*
		!CompatMeta
		for     UI
		with    void PushFont(Font@ font, float size)
		as      void PushFont(Font@ font, float size)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PushFontAs_Size(ScriptFont* Font, float Size)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushFont(Font ? Font->Get() : nullptr, Size);
		}
		UI_EXPORT("void PushFont(Font@+ font, float size)", PushFontAs_Size);

		/*
		!CompatMeta
		for     UI
		with    void PopFont()
		as      void PopFont()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialFn(PopFont);

		/*
		!CompatMeta
		for     UI
		with    void PushFontSize(float size)
		as      void PushFontSize(float size)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PushFontSizeAs(float Size)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushFont(nullptr, Size);
		}
		UI_EXPORT("void PushFontSize(float size)", PushFontSizeAs);

		/*
		!CompatMeta
		for     UI
		with    void PopFontSize()
		as      void PopFontSize()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PopFontSizeAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PopFont();
		}
		UI_EXPORT("void PopFontSize()", PopFontSizeAs);

		/*
		!CompatMeta
		for     UI
		with    float GetFontSize()
		as      float GetFontSize()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialReturnFn(GetFontSize, float);

		/*
		!CompatMeta
		for     UI
		with    int CalcWordWrapPosition(float size, const string&in text, float wrap_width, int text_offset = 0, Font@ font = null)
		as      int CalcWordWrapPosition(float size, const string&in text, float wrap_width, int text_offset = 0, Font@ font = null)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		int CalcWordWrapPositionAs(float Size, std::u8string& Text, float WrapWidth, int TextOffset, ScriptFont* Font)
		{
			Gecko_Exports_ImGui_GuardNonRender(0);

			ImFont* Using = Font ? Font->Get() : nullptr;
			if (!Using) Using = GetFont();

			const char* Begin = (const char*)Text.c_str();
			const char* End = Begin + Text.size();
			const char* From = Begin + std::clamp(TextOffset, 0, (int)Text.size());
			return (int)(Using->CalcWordWrapPosition(Size, From, End, WrapWidth) - Begin);
		}
		UI_EXPORT("int CalcWordWrapPosition(float size, const string&in text, float wrap_width, int text_offset = 0, Font@+ font = null)", CalcWordWrapPositionAs);

		/*
		!CompatMeta
		for     UI
		with    vec2 MeasureString(const string&in str, Font@ font = null, float size = 0.0f, float wrapWidth = 0.0f)
		as      vec2 MeasureString(const string&in str, Font@ font = null, float size = 0.0f, float wrapWidth = 0.0f)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Math::vec2 MeasureStringAs(std::u8string& Str, ScriptFont* Font, float Size, float WrapWidth)
		{
			Gecko_Exports_ImGui_GuardNonRender(Math::vec2{});

			ImFont* Using = Font ? Font->Get() : nullptr;
			if (!Using) Using = GetFont();
			if (Size <= 0.0f) Size = Font ? Font->Size : GetFontSize();

			const char* Begin = (const char*)Str.c_str();
			ImVec2 Measured = Using->CalcTextSizeA(Size, FLT_MAX, WrapWidth, Begin, Begin + Str.size());

			return { ceilf(Measured.x), Measured.y};
		}
		UI_EXPORT("vec2 MeasureString(const string&in str, Font@+ font = null, float size = 0.0f, float wrapWidth = 0.0f)", MeasureStringAs);
	}

	// Begin/End functions
	namespace
	{
		/*
		!CompatMeta
		for     UI
		with    bool Begin(const string&in title, int flags = UI::GetDefaultWindowFlags())
		as      bool Begin(const string&in title, int flags = UI::GetDefaultWindowFlags())
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginAs_NoOpen(std::u8string& title, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return Begin((const char*)title.c_str(), nullptr, flags);
		}
		UI_EXPORT("bool Begin(const string&in title, int flags = UI::GetDefaultWindowFlags())", BeginAs_NoOpen);

		/*
		!CompatMeta
		for     UI
		with    bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())
		as      bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginAs_Open(std::u8string& title, bool* open, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return Begin((const char*)title.c_str(), open, flags);
		}
		UI_EXPORT("bool Begin(const string&in title, bool&out open, int flags = UI::GetDefaultWindowFlags())", BeginAs_Open);

		/*
		!CompatMeta
		for     UI
		with    int GetDefaultWindowFlags()
		as      int GetDefaultWindowFlags()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		int GetDefaultWindowFlagsAs()
		{
			return TwinkUiState::DefaultWindowFlags();
		}
		UI_EXPORT("int GetDefaultWindowFlags()", GetDefaultWindowFlagsAs);

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
		Gecko_Exports_ImGui_MakeTrivialFn(End);

		/*
		!CompatMeta
		for     UI
		with    bool BeginCombo(const string&in label, const string&in current, int flags = UI::ComboFlags::None)
		as      bool BeginCombo(const string&in label, const string&in current, int flags = UI::ComboFlags::None)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginComboAs(std::u8string& label, std::u8string& current, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return BeginCombo((const char*)label.c_str(), (const char*)current.c_str(), flags);
		}
		UI_EXPORT("bool BeginCombo(const string&in label, const string&in current, int flags = UI::ComboFlags::None)", BeginComboAs);

		/*
		!CompatMeta
		for     UI
		with    void EndCombo()
		as      void EndCombo()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(Combo);

		/*
		!CompatMeta
		for     UI
		with    void BeginDisabled(bool disabled = true)
		as      void BeginDisabled(bool disabled = true)
		being   function
		declare full
		since   v0.2.0
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
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(Disabled);

		/*
		!CompatMeta
		for     UI
		with    void BeginTabBar(const string&in id, int flags = UI::TabBarFlags::None)
		as      void BeginTabBar(const string&in id, int flags = UI::TabBarFlags::None)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void BeginTabBarAs(std::u8string& id, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			BeginTabBar((const char*)id.c_str(), flags);
		}
		UI_EXPORT("void BeginTabBar(const string&in id, int flags = UI::TabBarFlags::None)", BeginTabBarAs);

		/*
		!CompatMeta
		for     UI
		with    void EndTabBar()
		as      void EndTabBar()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(TabBar);

		/*
		!CompatMeta
		for     UI
		with    bool BeginTabItem(const string&in label, int flags = UI::TabItemFlags::None)
		as      bool BeginTabItem(const string&in label, int flags = UI::TabItemFlags::None)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginTabItemAs_NoOpen(std::u8string& label, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return BeginTabItem((const char*)label.c_str(), nullptr, flags);
		}
		UI_EXPORT("bool BeginTabItem(const string&in label, int flags = UI::TabItemFlags::None)", BeginTabItemAs_NoOpen);

		/*
		!CompatMeta
		for     UI
		with    bool BeginTabItem(const string&in label, bool&out open, int flags = UI::TabItemFlags::None)
		as      bool BeginTabItem(const string&in label, bool&out open, int flags = UI::TabItemFlags::None)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginTabItemAs_Open(std::u8string& label, bool* open, int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return BeginTabItem((const char*)label.c_str(), open, flags);
		}
		UI_EXPORT("bool BeginTabItem(const string&in label, bool&out open, int flags = UI::TabItemFlags::None)", BeginTabItemAs_Open);

		/*
		!CompatMeta
		for     UI
		with    void EndTabItem()
		as      void EndTabItem()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(TabItem);

		/*
		!CompatMeta
		for     UI
		with    bool BeginTable(const string&in id, int columns, int flags = UI::TableFlags::None, const vec2&in outer_size = vec2(), float inner_width = 0.0f)
		as      bool BeginTable(const string&in id, int columns, int flags = UI::TableFlags::None, const vec2&in outer_size = vec2(), float inner_width = 0.0f)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool BeginTableAs(std::u8string& id, int columns, int flags, const Math::vec2& outer_size, float inner_width)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return BeginTable((const char*)id.c_str(), columns, flags, ImVec2(outer_size.x, outer_size.y), inner_width);
		}
		UI_EXPORT("bool BeginTable(const string&in id, int columns, int flags = UI::TableFlags::None, const vec2&in outer_size = vec2(), float inner_width = 0.0f)", BeginTableAs);

		/*
		!CompatMeta
		for     UI
		with    void EndTable()
		as      void EndTable()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(Table);

		/*
		!CompatMeta
		for     UI
		with    void BeginTooltip()
		as      void BeginTooltip()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialFn(BeginTooltip);

		/*
		!CompatMeta
		for     UI
		with    bool BeginItemTooltip()
		as      bool BeginItemTooltip()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialReturnFn(BeginItemTooltip, bool);

		/*
		!CompatMeta
		for     UI
		with    void EndTooltip()
		as      void EndTooltip()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeEndFn(Tooltip);
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
		void TextAs(std::u8string& s, int length)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			if (length != -1)
			{
				std::u8string_view vu(s);
				vu = vu.substr(0, length);
				Text("%.*s", vu.length(), (const char*)vu.data());
			}
			else
			{
				Text((const char*)s.c_str());
			}
		}
		UI_EXPORT("void Text(const string&in text, int length = -1)", TextAs);

		/*
		!CompatMeta
		for     UI
		with    void TextWrapped(const string&in text)
		as      void TextWrapped(const string&in text)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void TextWrappedAs(std::u8string& s)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			TextWrapped((const char*)s.c_str());
		}
		UI_EXPORT("void TextWrapped(const string&in text)", TextWrappedAs);

		/*
		!CompatMeta
		for     UI
		with    void TextDisabled(const string&in text)
		as      void TextDisabled(const string&in text)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void TextDisabledAs(std::u8string& s)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			TextDisabled((const char*)s.c_str());
		}
		UI_EXPORT("void TextDisabled(const string&in text)", TextDisabledAs);

		/*
		!CompatMeta
		for     UI
		with    bool TextLink(const string&in label)
		as      bool TextLink(const string&in label)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool TextLinkAs(std::u8string& s)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);

			return TextLink((const char*)s.c_str());
		}
		UI_EXPORT("bool TextLink(const string&in label)", TextLinkAs);

		/*
		!CompatMeta
		for     UI
		with    void TextLinkOpenURL(const string&in label, const string&in url)
		as      void TextLinkOpenURL(const string&in label, const string&in url)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void TextLinkOpenURLAs(std::u8string& l, std::u8string& t)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			TextLinkOpenURL((const char*)l.c_str(), (const char*)t.c_str());
		}
		UI_EXPORT("void TextLinkOpenURL(const string&in label, const string&in text)", TextLinkOpenURLAs);

		/*
		!CompatMeta
		for     UI
		with    void LabelText(const string&in label, const string&in text)
		as      void LabelText(const string&in label, const string&in text)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void LabelTextAs(std::u8string& l, std::u8string& t)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			LabelText((const char*)l.c_str(), (const char*)t.c_str());
		}
		UI_EXPORT("void LabelText(const string&in label, const string&in text)", LabelTextAs);

		/*
		!CompatMeta
		for     UI
		with    bool Button(const string&in label, const vec2&in size = vec2())
		as      bool Button(const string&in label, const vec2&in size = vec2())
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool ButtonAs(std::u8string& s, const Math::vec2& size)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return Button((const char*)s.c_str(), ImVec2(size.x, size.y));
		}
		UI_EXPORT("bool Button(const string&in label, const vec2&in size = vec2())", ButtonAs);

		/*
		!CompatMeta
		for     UI
		with    bool Checkbox(const string&in label, bool value)
		as      bool Checkbox(const string&in label, bool value)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool CheckboxAs(std::u8string& s, bool value)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			Checkbox((const char*)s.c_str(), &value);
			return value;
		}
		UI_EXPORT("bool Checkbox(const string&in label, bool value)", CheckboxAs);


		/*
		!CompatMeta
		for     UI
		with    void Image(Texture@ texture)
		as      void Image(Texture@ texture)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void ImageAs(Texture* Tex)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (!Tex) return;
			Image(Tex->Get(), Tex->GetSize());
		}
		UI_EXPORT("void Image(Texture@+ texture)", ImageAs);

		/*
		!CompatMeta
		for     UI
		with    void Image(Texture@ texture, const vec2&in size, const vec2&in uv0 = vec2(0, 0), const vec2&in uv1 = vec2(1, 1))
		as      void Image(Texture@ texture, const vec2&in size, const vec2&in uv0 = vec2(0, 0), const vec2&in uv1 = vec2(1, 1))
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void ImageSizedAs(Texture* Tex, const Math::vec2& size, const Math::vec2& uv0, const Math::vec2& uv1)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (!Tex) return;
			Image(Tex->Get(), ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
		}
		UI_EXPORT("void Image(Texture@+ texture, const vec2&in size, const vec2&in uv0 = vec2(0, 0), const vec2&in uv1 = vec2(1, 1))", ImageSizedAs);

		/*
		!CompatMeta
		for     UI
		with    vec2 GetSize()
		as      vec2 GetSize()
		being   method
		of      Texture
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Math::vec2 TextureGetSize(Texture* Tex)
		{
			ImVec2 Size = Tex->GetSize();
			return { Size.x, Size.y };
		}
		GECKO_EXPORT_METHOD(s_Exports, "UI", "Texture", "vec2 GetSize()", asFUNCTION(TextureGetSize), asCALL_CDECL_OBJLAST);
	}

	// Flags and state
	namespace 
	{
		/*
		!CompatMeta
		for     UI
		with    bool IsItemHovered(int flags = UI::HoveredFlags::None)
		as      bool IsItemHovered(int flags = UI::HoveredFlags::None)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool IsItemHoveredAs(int flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return IsItemHovered(flags);
		}
		UI_EXPORT("bool IsItemHovered(int flags = UI::HoveredFlags::None)", IsItemHoveredAs);

		/*
		!CompatMeta
		for     UI
		with    bool IsItemActive()
		as      bool IsItemActive()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialReturnFn(IsItemActive, bool);

		/*
		!CompatMeta
		for     UI
		with    bool IsItemFocused()
		as      bool IsItemFocused()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialReturnFn(IsItemFocused, bool);

		/*
		!CompatMeta
		for     UI
		with    void PushID(const string&in id)
		as      void PushID(const string&in id)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PushIDAs(std::u8string& id)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushID((const char*)id.c_str());
		}
		UI_EXPORT("void PushID(const string&in id)", PushIDAs);

		/*
		!CompatMeta
		for     UI
		with    void PopID()
		as      void PopID()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialFn(PopID);

		/*
		!CompatMeta
		for     UI
		with    void PushItemWidth(float item_width)
		as      void PushItemWidth(float item_width)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void PushItemWidthAs(float w)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushItemWidth(w);
		}
		UI_EXPORT("void PushItemWidth(float item_width)", PushItemWidthAs);

		/*
		!CompatMeta
		for     UI
		with    void PopItemWidth()
		as      void PopItemWidth()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialFn(PopItemWidth);

		/*
		!CompatMeta
		for     UI
		with    void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f)
		as      void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void SameLineAs(float ox, float sp)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SameLine(ox, sp);
		}
		UI_EXPORT("void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f)", SameLineAs);

		/*
		!CompatMeta
		for     UI
		with    void Separator()
		with    void Separator(int flags, float thickness = 1.0f)
		as      void Separator()
		as      void Separator(int flags, float thickness = 1.0f)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialFn(Separator);

		void SeparatorExAs(int flags, float thickness)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			// Dear ImGui asserts on a separator that's neither horizontal nor vertical
			if (not (flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))) flags |= ImGuiSeparatorFlags_Horizontal;
			SeparatorEx(flags, thickness);
		}
		UI_EXPORT("void Separator(int flags, float thickness = 1.0f)", SeparatorExAs);
	}

	namespace
	{
		using Math::vec2;
		using Math::vec4;

		void ScriptError(const char* Message)
		{
			if (auto Ctx = asGetActiveContext()) Ctx->SetException(Message);
		}

		ImVec2 ToIm(const vec2& V) { return { V.x, V.y }; }
		ImVec4 ToIm(const vec4& V) { return { V.x, V.y, V.z, V.w }; }
		vec2 FromIm(const ImVec2& V) { return { V.x, V.y }; }
		vec4 FromIm(const ImVec4& V) { return { V.x, V.y, V.z, V.w }; }

		/*
		!CompatMeta
		for     UI
		with    void PushStyleColor(Col idx, const vec4&in col)
		with    void PopStyleColor(int count = 1)
		with    vec4 GetStyleColor(Col idx)
		with    void PushStyleVar(StyleVar var, float value)
		with    void PushStyleVar(StyleVar var, const vec2&in value)
		with    void PopStyleVar(int count = 1)
		with    float GetStyleVarFloat(StyleVar var)
		with    vec2 GetStyleVarVec2(StyleVar var)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool ValidCol(int Idx) { return Idx >= 0 and Idx < ImGuiCol_COUNT; }

		const ImGuiStyleVarInfo* StyleVarInfo(int Var, int Count)
		{
			if (Var < 0 or Var >= ImGuiStyleVar_COUNT) return nullptr;
			const ImGuiStyleVarInfo* Info = GetStyleVarInfo(Var);
			return Info->DataType == ImGuiDataType_Float and (int)Info->Count == Count ? Info : nullptr;
		}

		void PushStyleColorAs(int Idx, const vec4& Col)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (ValidCol(Idx)) PushStyleColor(Idx, ToIm(Col));
		}
		UI_EXPORT("void PushStyleColor(Col idx, const vec4&in col)", PushStyleColorAs);

		void PopStyleColorAs(int Count)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Count = (std::min)(Count, GImGui->ColorStack.Size);
			if (Count > 0) PopStyleColor(Count);
		}
		UI_EXPORT("void PopStyleColor(int count = 1)", PopStyleColorAs);

		vec4 GetStyleColorAs(int Idx)
		{
			if (not ::ImGui::GetCurrentContext() or not ValidCol(Idx)) return {};
			return FromIm(GetStyle().Colors[Idx]);
		}
		UI_EXPORT("vec4 GetStyleColor(Col idx)", GetStyleColorAs);

		void PushStyleVarFloatAs(int Var, float Value)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (StyleVarInfo(Var, 1)) PushStyleVar(Var, Value);
			else ScriptError("This style variable isn't a float");
		}
		UI_EXPORT("void PushStyleVar(StyleVar var, float value)", PushStyleVarFloatAs);

		void PushStyleVarVec2As(int Var, const vec2& Value)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (StyleVarInfo(Var, 2)) PushStyleVar(Var, ToIm(Value));
			else ScriptError("This style variable isn't a vec2");
		}
		UI_EXPORT("void PushStyleVar(StyleVar var, const vec2&in value)", PushStyleVarVec2As);

		void PopStyleVarAs(int Count)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Count = (std::min)(Count, GImGui->StyleVarStack.Size);
			if (Count > 0) PopStyleVar(Count);
		}
		UI_EXPORT("void PopStyleVar(int count = 1)", PopStyleVarAs);

		float GetStyleVarFloatAs(int Var)
		{
			if (not ::ImGui::GetCurrentContext()) return 0.0f;
			auto Info = StyleVarInfo(Var, 1);
			return Info ? *(float*)Info->GetVarPtr(&GetStyle()) : 0.0f;
		}
		UI_EXPORT("float GetStyleVarFloat(StyleVar var)", GetStyleVarFloatAs);

		vec2 GetStyleVarVec2As(int Var)
		{
			if (not ::ImGui::GetCurrentContext()) return {};
			auto Info = StyleVarInfo(Var, 2);
			return Info ? FromIm(*(ImVec2*)Info->GetVarPtr(&GetStyle())) : vec2{};
		}
		UI_EXPORT("vec2 GetStyleVarVec2(StyleVar var)", GetStyleVarVec2As);

		/*
		!CompatMeta
		for     UI
		with    void SetNextWindowSize(int w, int h, Cond cond = UI::Cond::Appearing)
		with    void SetNextWindowPos(int x, int y, Cond cond = UI::Cond::Appearing, float pivotx = 0.0f, float pivoty = 0.0f)
		with    void SetNextWindowContentSize(int w, int h = 0)
		with    void SetNextWindowSizeConstraints(int min_width, int min_height, int max_width, int max_height)
		with    void SetNextWindowCollapsed(bool collapsed, Cond cond = UI::Cond::None)
		with    void SetNextWindowBgAlpha(float alpha)
		with    bool IsWindowAppearing()
		with    vec2 GetWindowPos()
		with    void SetWindowPos(const vec2&in size, Cond cond = UI::Cond::None)
		with    vec2 GetWindowSize()
		with    void SetWindowSize(const vec2&in size, Cond cond = UI::Cond::None)
		with    bool IsWindowFocused(int flags = UI::FocusedFlags::None)
		with    vec2 GetContentRegionAvail()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void SetNextWindowSizeAs(int W, int H, int Cond)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowSize(ImVec2((float)W, (float)H), Cond);
		}
		UI_EXPORT("void SetNextWindowSize(int w, int h, Cond cond = UI::Cond::Appearing)", SetNextWindowSizeAs);

		void SetNextWindowPosAs(int X, int Y, int Cond, float PivotX, float PivotY)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowPos(ImVec2((float)X, (float)Y), Cond, ImVec2(PivotX, PivotY));
		}
		UI_EXPORT("void SetNextWindowPos(int x, int y, Cond cond = UI::Cond::Appearing, float pivotx = 0.0f, float pivoty = 0.0f)", SetNextWindowPosAs);

		void SetNextWindowContentSizeAs(int W, int H)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowContentSize(ImVec2((float)W, (float)H));
		}
		UI_EXPORT("void SetNextWindowContentSize(int w, int h = 0)", SetNextWindowContentSizeAs);

		void SetNextWindowSizeConstraintsAs(int MinW, int MinH, int MaxW, int MaxH)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowSizeConstraints(ImVec2((float)MinW, (float)MinH), ImVec2((float)MaxW, (float)MaxH));
		}
		UI_EXPORT("void SetNextWindowSizeConstraints(int min_width, int min_height, int max_width, int max_height)", SetNextWindowSizeConstraintsAs);

		void SetNextWindowCollapsedAs(bool Collapsed, int Cond)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowCollapsed(Collapsed, Cond);
		}
		UI_EXPORT("void SetNextWindowCollapsed(bool collapsed, Cond cond = UI::Cond::None)", SetNextWindowCollapsedAs);

		void SetNextWindowBgAlphaAs(float Alpha)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetNextWindowBgAlpha(Alpha);
		}
		UI_EXPORT("void SetNextWindowBgAlpha(float alpha)", SetNextWindowBgAlphaAs);

		Gecko_Exports_ImGui_MakeTrivialReturnFn(IsWindowAppearing, bool);

		vec2 GetWindowPosAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(vec2{});
			return FromIm(GetWindowPos());
		}
		UI_EXPORT("vec2 GetWindowPos()", GetWindowPosAs);

		void SetWindowPosAs(const vec2& Pos, int Cond)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetWindowPos(ToIm(Pos), Cond);
		}
		UI_EXPORT("void SetWindowPos(const vec2&in size, Cond cond = UI::Cond::None)", SetWindowPosAs);

		vec2 GetWindowSizeAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(vec2{});
			return FromIm(GetWindowSize());
		}
		UI_EXPORT("vec2 GetWindowSize()", GetWindowSizeAs);

		void SetWindowSizeAs(const vec2& Size, int Cond)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetWindowSize(ToIm(Size), Cond);
		}
		UI_EXPORT("void SetWindowSize(const vec2&in size, Cond cond = UI::Cond::None)", SetWindowSizeAs);

		bool IsWindowFocusedAs(int Flags)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return IsWindowFocused(Flags);
		}
		UI_EXPORT("bool IsWindowFocused(int flags = UI::FocusedFlags::None)", IsWindowFocusedAs);

		vec2 GetContentRegionAvailAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(vec2{});
			return FromIm(GetContentRegionAvail());
		}
		UI_EXPORT("vec2 GetContentRegionAvail()", GetContentRegionAvailAs);

		/*
		!CompatMeta
		for     UI
		with    void Dummy(const vec2&in size)
		with    void Dummy(const vec2&in size, float text_baseline_y)
		with    void PushTextWrapPos(float wrap_local_pos_x = 0.0f)
		with    void PopTextWrapPos()
		with    vec2 GetCursorPos()
		with    void SetCursorPos(const vec2&in pos)
		with    void SetCursorPosX(float x)
		with    void SetCursorPosY(float y)
		with    vec2 GetCursorScreenPos()
		with    void SetCursorScreenPos(const vec2&in pos)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		void DummyAs(const vec2& Size)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Dummy(ToIm(Size));
		}
		UI_EXPORT("void Dummy(const vec2&in size)", DummyAs);

		void DummyBaselineAs(const vec2& Size, float TextBaselineY)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			ImGuiWindow* Window = GetCurrentWindow();
			if (Window->SkipItems) return;

			const ImRect Bb(Window->DC.CursorPos, Window->DC.CursorPos + ToIm(Size));
			ItemSize(ToIm(Size), TextBaselineY);
			ItemAdd(Bb, 0);
		}
		UI_EXPORT("void Dummy(const vec2&in size, float text_baseline_y)", DummyBaselineAs);

		void PushTextWrapPosAs(float WrapLocalPosX)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			PushTextWrapPos(WrapLocalPosX);
		}
		UI_EXPORT("void PushTextWrapPos(float wrap_local_pos_x = 0.0f)", PushTextWrapPosAs);

		void PopTextWrapPosAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (not GetCurrentWindow()->DC.TextWrapPosStack.empty()) PopTextWrapPos();
		}
		UI_EXPORT("void PopTextWrapPos()", PopTextWrapPosAs);

		vec2 GetCursorPosAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(vec2{});
			return FromIm(GetCursorPos());
		}
		UI_EXPORT("vec2 GetCursorPos()", GetCursorPosAs);

		void SetCursorPosAs(const vec2& Pos)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetCursorPos(ToIm(Pos));
		}
		UI_EXPORT("void SetCursorPos(const vec2&in pos)", SetCursorPosAs);

		void SetCursorPosXAs(float X)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetCursorPosX(X);
		}
		UI_EXPORT("void SetCursorPosX(float x)", SetCursorPosXAs);

		void SetCursorPosYAs(float Y)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetCursorPosY(Y);
		}
		UI_EXPORT("void SetCursorPosY(float y)", SetCursorPosYAs);

		vec2 GetCursorScreenPosAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(vec2{});
			return FromIm(GetCursorScreenPos());
		}
		UI_EXPORT("vec2 GetCursorScreenPos()", GetCursorScreenPosAs);

		void SetCursorScreenPosAs(const vec2& Pos)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			SetCursorScreenPos(ToIm(Pos));
		}
		UI_EXPORT("void SetCursorScreenPos(const vec2&in pos)", SetCursorScreenPosAs);

		/*
		!CompatMeta
		for     UI
		with    bool BeginMenuBar()
		with    void EndMenuBar()
		with    bool BeginMenu(const string&in label, bool enabled = true)
		with    void EndMenu()
		with    bool MenuItem(const string&in label, const string&in shortcut = "", bool selected = false, bool enabled = true)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Gecko_Exports_ImGui_MakeTrivialReturnFn(BeginMenuBar, bool);
		Gecko_Exports_ImGui_MakeEndFn(MenuBar);

		bool BeginMenuAs(std::u8string& Label, bool Enabled)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return BeginMenu((const char*)Label.c_str(), Enabled);
		}
		UI_EXPORT("bool BeginMenu(const string&in label, bool enabled = true)", BeginMenuAs);

		void EndMenuAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			::ImGui::EndMenu();
		}
		UI_EXPORT("void EndMenu()", EndMenuAs);

		bool MenuItemAs(std::u8string& Label, std::u8string& Shortcut, bool Selected, bool Enabled)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return MenuItem((const char*)Label.c_str(), Shortcut.empty() ? nullptr : (const char*)Shortcut.c_str(), Selected, Enabled);
		}
		UI_EXPORT("bool MenuItem(const string&in label, const string&in shortcut = \"\", bool selected = false, bool enabled = true)", MenuItemAs);

		/*
		!CompatMeta
		for     UI
		with    void TableNextRow(int flags = UI::TableRowFlags::None, float min_height = 0.0f)
		with    bool TableNextColumn()
		with    bool TableSetColumnIndex(int index)
		with    void TableSetupColumn(const string&in label, int flags = UI::TableColumnFlags::None, float init_width_or_weight = 0.0f, uint user_id = 0)
		with    void TableSetupScrollFreeze(int cols, int rows)
		with    void TableHeadersRow()
		with    void TableHeader(const string&in label)
		with    int TableGetColumnCount()
		with    int TableGetColumnIndex()
		with    int TableGetRowIndex()
		with    void TableSetBgColor(TableBgTarget target, const vec4&in color, int column_n = -1)
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool InTable() { return GetCurrentTable() != nullptr; }

		void TableNextRowAs(int Flags, float MinHeight)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (InTable()) TableNextRow(Flags, MinHeight);
		}
		UI_EXPORT("void TableNextRow(int flags = UI::TableRowFlags::None, float min_height = 0.0f)", TableNextRowAs);

		bool TableNextColumnAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			return InTable() and TableNextColumn();
		}
		UI_EXPORT("bool TableNextColumn()", TableNextColumnAs);

		bool TableSetColumnIndexAs(int Index)
		{
			Gecko_Exports_ImGui_GuardNonRender(false);
			if (not InTable() or Index < 0 or Index >= TableGetColumnCount()) return false;
			return TableSetColumnIndex(Index);
		}
		UI_EXPORT("bool TableSetColumnIndex(int index)", TableSetColumnIndexAs);

		void TableSetupColumnAs(std::u8string& Label, int Flags, float InitWidthOrWeight, uint32_t UserId)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			ImGuiTable* Table = GetCurrentTable();

			if (not Table or Table->DeclColumnsCount >= Table->ColumnsCount) return;
			TableSetupColumn((const char*)Label.c_str(), Flags, InitWidthOrWeight, UserId);
		}
		UI_EXPORT("void TableSetupColumn(const string&in label, int flags = UI::TableColumnFlags::None, float init_width_or_weight = 0.0f, uint user_id = 0)", TableSetupColumnAs);

		void TableSetupScrollFreezeAs(int Cols, int Rows)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (InTable()) TableSetupScrollFreeze(Cols, Rows);
		}
		UI_EXPORT("void TableSetupScrollFreeze(int cols, int rows)", TableSetupScrollFreezeAs);

		void TableHeadersRowAs()
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (InTable()) TableHeadersRow();
		}
		UI_EXPORT("void TableHeadersRow()", TableHeadersRowAs);

		void TableHeaderAs(std::u8string& Label)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (InTable()) TableHeader((const char*)Label.c_str());
		}
		UI_EXPORT("void TableHeader(const string&in label)", TableHeaderAs);

		Gecko_Exports_ImGui_MakeTrivialReturnFn(TableGetColumnCount, int);
		Gecko_Exports_ImGui_MakeTrivialReturnFn(TableGetColumnIndex, int);
		Gecko_Exports_ImGui_MakeTrivialReturnFn(TableGetRowIndex, int);

		void TableSetBgColorAs(int Target, const vec4& Color, int Column)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (InTable()) TableSetBgColor(Target, ColorConvertFloat4ToU32(ToIm(Color)), Column);
		}
		UI_EXPORT("void TableSetBgColor(TableBgTarget target, const vec4&in color, int column_n = -1)", TableSetBgColorAs);

		/*
		!CompatMeta
		for     UI
		with    bool IsOverlayShown()
		with    void ShowOverlay()
		with    void HideOverlay()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool IsOverlayShownAs() { return TwinkUiState::RenderUi; }
		UI_EXPORT("bool IsOverlayShown()", IsOverlayShownAs);

		void ShowOverlayAs() { TwinkUiState::RenderUi = true; }
		UI_EXPORT("void ShowOverlay()", ShowOverlayAs);

		void HideOverlayAs() { TwinkUiState::RenderUi = false; }
		UI_EXPORT("void HideOverlay()", HideOverlayAs);

		/*
		!CompatMeta
		for     UI
		with    bool IsGameUIVisible()
		being   function
		declare partial
		reason  Always true for now
		since   v0.2.0
		!CompatMeta
		*/
		bool IsGameUIVisibleAs() { return true; }
		UI_EXPORT("bool IsGameUIVisible()", IsGameUIVisibleAs);

		/*
		!CompatMeta
		for     UI
		with    bool IsDockingEnabled()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		bool IsDockingEnabledAs() { return GetCurrentContext() && (GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable); }
		UI_EXPORT("bool IsDockingEnabled()", IsDockingEnabledAs);
	}

	// UI::DrawList
	namespace
	{
		using Math::vec2;
		using Math::vec4;

#define DRAWLIST_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "UI", "DrawList", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

		ImU32 Col(const vec4& C) { return ColorConvertFloat4ToU32(ImVec4(C.x, C.y, C.z, C.w)); }
		ImVec2 Min(const vec4& Rect) { return { Rect.x, Rect.y }; }
		ImVec2 Max(const vec4& Rect) { return { Rect.x + Rect.z, Rect.y + Rect.w }; }

		ImU32 ColRgba(uint32_t C) { return IM_COL32((C >> 24) & 0xFF, (C >> 16) & 0xFF, (C >> 8) & 0xFF, C & 0xFF); }

		/*
		!CompatMeta
		for     UI
		with    DrawList@ GetBackgroundDrawList()
		with    DrawList@ GetForegroundDrawList()
		with    DrawList@ GetWindowDrawList()
		being   function
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		ImDrawList* GetBackgroundDrawListAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(nullptr);
			return GetBackgroundDrawList();
		}
		UI_EXPORT("DrawList@ GetBackgroundDrawList()", GetBackgroundDrawListAs);

		ImDrawList* GetForegroundDrawListAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(nullptr);
			return GetForegroundDrawList();
		}
		UI_EXPORT("DrawList@ GetForegroundDrawList()", GetForegroundDrawListAs);

		ImDrawList* GetWindowDrawListAs()
		{
			Gecko_Exports_ImGui_GuardNonRender(nullptr);
			return GetWindowDrawList();
		}
		UI_EXPORT("DrawList@ GetWindowDrawList()", GetWindowDrawListAs);

		static int& DL_ClipRectPushes(ImDrawList* Self)
		{
			static std::unordered_map<ImDrawList*, int> Pushes;
			static int Frame = -1;
			if (Frame != GetFrameCount())
			{
				Pushes.clear();
				Frame = GetFrameCount();
			}
			return Pushes[Self];
		}

		void DL_PushClipRect(const vec4& Rect, bool Intersect, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->PushClipRect(Min(Rect), Max(Rect), Intersect);
			DL_ClipRectPushes(Self)++;
		}

		void DL_PushClipRectFullScreen(ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->PushClipRectFullScreen();
			DL_ClipRectPushes(Self)++;
		}

		void DL_PopClipRect(ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			int& Pushes = DL_ClipRectPushes(Self);
			if (Pushes <= 0 || Self->_ClipRectStack.Size <= 1) return;
			Self->PopClipRect();
			Pushes--;
		}

		void DL_AddRect(const vec4& Rect, const vec4& Color, float Rounding, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddRect(Min(Rect), Max(Rect), Col(Color), Rounding, 0, Thickness);
		}

		void DL_AddRectFilled(const vec4& Rect, const vec4& Color, float Rounding, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddRectFilled(Min(Rect), Max(Rect), Col(Color), Rounding);
		}

		void DL_AddRectFilledMultiColor(const vec4& Rect, const vec4& UL, const vec4& UR, const vec4& BL, const vec4& BR, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddRectFilledMultiColor(Min(Rect), Max(Rect), Col(UL), Col(UR), Col(BR), Col(BL));
		}

		void DL_AddQuad(const vec2& TL, const vec2& TR, const vec2& BR, const vec2& BL, const vec4& Color, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddQuad(ToIm(TL), ToIm(TR), ToIm(BR), ToIm(BL), Col(Color), Thickness);
		}

		void DL_AddQuadFilled(const vec2& TL, const vec2& TR, const vec2& BR, const vec2& BL, const vec4& Color, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddQuadFilled(ToIm(TL), ToIm(TR), ToIm(BR), ToIm(BL), Col(Color));
		}

		void DL_AddTriangle(const vec2& P1, const vec2& P2, const vec2& P3, const vec4& Color, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddTriangle(ToIm(P1), ToIm(P2), ToIm(P3), Col(Color), Thickness);
		}

		void DL_AddTriangleFilled(const vec2& P1, const vec2& P2, const vec2& P3, const vec4& Color, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddTriangleFilled(ToIm(P1), ToIm(P2), ToIm(P3), Col(Color));
		}

		void DL_AddCircle(const vec2& Pos, float Radius, const vec4& Color, int Segments, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddCircle(ToIm(Pos), Radius, Col(Color), Segments, Thickness);
		}

		void DL_AddCircleFilled(const vec2& Pos, float Radius, const vec4& Color, int Segments, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddCircleFilled(ToIm(Pos), Radius, Col(Color), Segments);
		}

		void DL_AddNgon(const vec2& Pos, float Radius, const vec4& Color, int Segments, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (Segments > 2) Self->AddNgon(ToIm(Pos), Radius, Col(Color), Segments, Thickness);
		}

		void DL_AddNgonFilled(const vec2& Pos, float Radius, const vec4& Color, int Segments, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (Segments > 2) Self->AddNgonFilled(ToIm(Pos), Radius, Col(Color), Segments);
		}

		void DL_AddLine(const vec2& A, const vec2& B, const vec4& Color, float Thickness, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddLine(ToIm(A), ToIm(B), Col(Color), Thickness);
		}

		void DL_AddBezierCubic(const vec2& P1, const vec2& P2, const vec2& P3, const vec2& P4, const vec4& Color, float Thickness, int Segments, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddBezierCubic(ToIm(P1), ToIm(P2), ToIm(P3), ToIm(P4), Col(Color), Thickness, Segments);
		}

		void DL_AddBezierQuadratic(const vec2& P1, const vec2& P2, const vec2& P3, const vec4& Color, float Thickness, int Segments, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			Self->AddBezierQuadratic(ToIm(P1), ToIm(P2), ToIm(P3), Col(Color), Thickness, Segments);
		}

		void DL_AddText(const vec2& Pos, const vec4& Color, std::u8string& Str, ScriptFont* Font, float Size, float WrapWidth, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();

			ImFont* Using = Font ? Font->Get() : nullptr;
			if (!Using) Using = GetFont();
			if (Size <= 0.0f) Size = Font ? Font->Size : GetFontSize();

			const char* Begin = (const char*)Str.c_str();
			Self->AddText(Using, Size, ToIm(Pos), Col(Color), Begin, Begin + Str.size(), WrapWidth);
		}

		void DL_AddImageUv(Texture* Tex, const vec2& Pos, const vec2& Size, ImU32 Color, ImVec2 Uv0, ImVec2 Uv1, ImDrawList* Self)
		{
			Gecko_Exports_ImGui_GuardNonRender();
			if (Tex) Self->AddImage(Tex->Get(), ToIm(Pos), ToIm(Pos) + ToIm(Size), Uv0, Uv1, Color);
		}

		void DL_AddImage(Texture* Tex, const vec2& Pos, ImDrawList* Self)
		{
			if (Tex) DL_AddImageUv(Tex, Pos, FromIm(Tex->GetSize()), IM_COL32_WHITE, { 0, 0 }, { 1, 1 }, Self);
		}

		void DL_AddImageSized(Texture* Tex, const vec2& Pos, const vec2& Size, ImDrawList* Self)
		{
			DL_AddImageUv(Tex, Pos, Size, IM_COL32_WHITE, { 0, 0 }, { 1, 1 }, Self);
		}

		void DL_AddImageColored(Texture* Tex, const vec2& Pos, const vec2& Size, uint32_t Color, ImDrawList* Self)
		{
			DL_AddImageUv(Tex, Pos, Size, ColRgba(Color), { 0, 0 }, { 1, 1 }, Self);
		}

		void DL_AddImagePart(Texture* Tex, const vec2& Pos, const vec2& Size, uint32_t Color, const vec4& Uv, ImDrawList* Self)
		{
			if (!Tex) return;
			ImVec2 TexSize = Tex->GetSize();
			if (TexSize.x <= 0 or TexSize.y <= 0) return;
			DL_AddImageUv(Tex, Pos, Size, ColRgba(Color), Min(Uv) / TexSize, Max(Uv) / TexSize, Self);
		}

		/*
		!CompatMeta
		for     UI
		with    void PushClipRect(const vec4&in rect, bool intersectWithCurrentClipRect = false)
		with    void PushClipRectFullScreen()
		with    void PopClipRect()
		with    void AddRect(const vec4&in rect, const vec4&in color, float rounding = 0.0f, float thickness = 1.0f)
		with    void AddRectFilled(const vec4&in rect, const vec4&in color, float rounding = 0.0f)
		with    void AddRectFilledMultiColor(const vec4&in rect, const vec4&in colUL, const vec4&in colUR, const vec4&in colBL, const vec4&in colBR)
		with    void AddQuad(const vec2&in topLeft, const vec2&in topRight, const vec2&in bottomRight, const vec2&in bottomLeft, const vec4&in color, float thickness = 1.0f)
		with    void AddQuadFilled(const vec2&in topLeft, const vec2&in topRight, const vec2&in bottomRight, const vec2&in bottomLeft, const vec4&in color)
		with    void AddTriangle(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color, float thickness = 1.0f)
		with    void AddQuadFilled(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color)
		with    void AddCircle(const vec2&in pos, float radius, const vec4&in color, int segments = 0, float thickness = 1.0f)
		with    void AddCircleFilled(const vec2&in pos, float radius, const vec4&in color, int segments = 0)
		with    void AddNgon(const vec2&in pos, float radius, const vec4&in color, int segments, float thickness = 1.0f)
		with    void AddNgonFilled(const vec2&in pos, float radius, const vec4&in color, int segments)
		with    void AddLine(const vec2&in a, const vec2&in b, const vec4&in color, float thickness = 1.0f)
		with    void AddBezierCubic(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec2&in p4, const vec4&in color, float thickness = 1.0f, int segments = 0)
		with    void AddBezierQuadratic(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color, float thickness = 1.0f, int segments = 0)
		with    void AddText(const vec2&in pos, const vec4&in color, const string&in str, UI::Font@ font = null, float size = 0.0f, float wrapWidth = 0.0f)
		with    void AddImage(UI::Texture@ texture, const vec2&in pos)
		with    void AddImage(UI::Texture@ texture, const vec2&in pos, const vec2&in size)
		with    void AddImage(UI::Texture@ texture, const vec2&in pos, const vec2&in size, uint color)
		with    void AddImage(UI::Texture@ texture, const vec2&in pos, const vec2&in size, uint color, const vec4&in uv)
		being   method
		of      DrawList
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		DRAWLIST_METHOD("void PushClipRect(const vec4&in rect, bool intersectWithCurrentClipRect = false)", DL_PushClipRect);
		DRAWLIST_METHOD("void PushClipRectFullScreen()", DL_PushClipRectFullScreen);
		DRAWLIST_METHOD("void PopClipRect()", DL_PopClipRect);
		DRAWLIST_METHOD("void AddRect(const vec4&in rect, const vec4&in color, float rounding = 0.0f, float thickness = 1.0f)", DL_AddRect);
		DRAWLIST_METHOD("void AddRectFilled(const vec4&in rect, const vec4&in color, float rounding = 0.0f)", DL_AddRectFilled);
		DRAWLIST_METHOD("void AddRectFilledMultiColor(const vec4&in rect, const vec4&in colUL, const vec4&in colUR, const vec4&in colBL, const vec4&in colBR)", DL_AddRectFilledMultiColor);
		DRAWLIST_METHOD("void AddQuad(const vec2&in topLeft, const vec2&in topRight, const vec2&in bottomRight, const vec2&in bottomLeft, const vec4&in color, float thickness = 1.0f)", DL_AddQuad);
		DRAWLIST_METHOD("void AddQuadFilled(const vec2&in topLeft, const vec2&in topRight, const vec2&in bottomRight, const vec2&in bottomLeft, const vec4&in color)", DL_AddQuadFilled);
		DRAWLIST_METHOD("void AddTriangle(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color, float thickness = 1.0f)", DL_AddTriangle);
		DRAWLIST_METHOD("void AddQuadFilled(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color)", DL_AddTriangleFilled);
		DRAWLIST_METHOD("void AddCircle(const vec2&in pos, float radius, const vec4&in color, int segments = 0, float thickness = 1.0f)", DL_AddCircle);
		DRAWLIST_METHOD("void AddCircleFilled(const vec2&in pos, float radius, const vec4&in color, int segments = 0)", DL_AddCircleFilled);
		DRAWLIST_METHOD("void AddNgon(const vec2&in pos, float radius, const vec4&in color, int segments, float thickness = 1.0f)", DL_AddNgon);
		DRAWLIST_METHOD("void AddNgonFilled(const vec2&in pos, float radius, const vec4&in color, int segments)", DL_AddNgonFilled);
		DRAWLIST_METHOD("void AddLine(const vec2&in a, const vec2&in b, const vec4&in color, float thickness = 1.0f)", DL_AddLine);
		DRAWLIST_METHOD("void AddBezierCubic(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec2&in p4, const vec4&in color, float thickness = 1.0f, int segments = 0)", DL_AddBezierCubic);
		DRAWLIST_METHOD("void AddBezierQuadratic(const vec2&in p1, const vec2&in p2, const vec2&in p3, const vec4&in color, float thickness = 1.0f, int segments = 0)", DL_AddBezierQuadratic);
		DRAWLIST_METHOD("void AddText(const vec2&in pos, const vec4&in color, const string&in str, Font@+ font = null, float size = 0.0f, float wrapWidth = 0.0f)", DL_AddText);
		DRAWLIST_METHOD("void AddImage(Texture@+ texture, const vec2&in pos)", DL_AddImage);
		DRAWLIST_METHOD("void AddImage(Texture@+ texture, const vec2&in pos, const vec2&in size)", DL_AddImageSized);
		DRAWLIST_METHOD("void AddImage(Texture@+ texture, const vec2&in pos, const vec2&in size, uint color)", DL_AddImageColored);
		DRAWLIST_METHOD("void AddImage(Texture@+ texture, const vec2&in pos, const vec2&in size, uint color, const vec4&in uv)", DL_AddImagePart);
	}

	void Registrar(asIScriptEngine* Engine)
	{
		// First, since some functions' default arguments use them
		RegisterEnums(Engine);

		Engine->SetDefaultNamespace("UI");
		Engine->RegisterObjectType("Texture", sizeof(Texture), asOBJ_REF);

		/*
		!CompatMeta
		for     UI
		with    void $beh5()
		as      void $beh5()
		being   behavior
		of      Texture
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectBehaviour("Texture", asBEHAVE_ADDREF, "void f()", asMETHOD(Texture, AddRef), asCALL_THISCALL);

		/*
		!CompatMeta
		for     UI
		with    void $beh6()
		as      void $beh6()
		being   behavior
		of      Texture
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectBehaviour("Texture", asBEHAVE_RELEASE, "void f()", asMETHOD(Texture, RemoveRef), asCALL_THISCALL);

		Engine->RegisterObjectType("Font", 0, asOBJ_REF);
		Engine->RegisterObjectBehaviour("Font", asBEHAVE_ADDREF, "void f()", asMETHOD(ScriptFont, AddRef), asCALL_THISCALL);
		Engine->RegisterObjectBehaviour("Font", asBEHAVE_RELEASE, "void f()", asMETHOD(ScriptFont, RemoveRef), asCALL_THISCALL);

		Engine->RegisterObjectType("DrawList", 0, asOBJ_REF | asOBJ_NOCOUNT);
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
		RegisterPlot(Engine);
	}
}
