#include "pch.h"
#include "ExportNvg.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/Exports/Math/Math.h>
#include <Gecko/Exports/Global/MemoryBuffer/MemoryBuffer.h>
#include <Gecko/ScriptFiles.h>
#include <Twinkie/TwinkNvg/TwinkNvg.h>
#include <Twinkie/TwinkUi/TwinkUi.h>
#include <fstream>

namespace Gecko::Exports::Nvg
{
	using Math::vec2;
	using Math::vec3;
	using Math::vec4;
	using Math::mat3;

	static std::vector<GeckoExport> s_Exports;
#define NVG_FN(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "nvg", Decl_, Fn_)

	static void Warn(const std::string& Message)
	{
		LoggertAngelscript.warn(std::u8string(Message.begin(), Message.end()));
	}

	static bool ReadFile(const std::filesystem::path& Path, std::vector<uint8_t>& Out)
	{
		std::error_code Error;
		if (not std::filesystem::is_regular_file(Path, Error)) return false;

		std::ifstream File(Path, std::ios::binary);
		if (not File) return false;

		Out.assign(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
		return true;
	}

	static bool ReadScriptOrFontsFile(const std::string& Filename, std::vector<uint8_t>& Out)
	{
		std::filesystem::path Relative = std::filesystem::path((const char8_t*)Filename.c_str()).lexically_normal();
		std::string Key;
		if (Gecko::ReadScriptFile(Relative, Out, &Key) and not Key.empty()) return true;
		return TwinkUiState::UiMgr and ReadFile(TwinkUiState::UiMgr->DocumentsFolderPath / L"TwinkieX\\Fonts" / Relative, Out);
	}

	struct FontEntry
	{
		std::string Name;
		std::vector<uint8_t> Data;
		std::vector<int> Fallbacks;
	};

	static std::vector<std::unique_ptr<FontEntry>> Fonts;
	static std::vector<int> NvgFontIds;
	static uint32_t FontsGeneration = 0;
	static int IconsFont = -2;

	static int AddFont(std::string Name, std::vector<uint8_t> Data)
	{
		auto Entry = std::make_unique<FontEntry>();
		Entry->Name = std::move(Name);
		Entry->Data = std::move(Data);
		Fonts.push_back(std::move(Entry));
		return (int)Fonts.size() - 1;
	}

	static int IconsFallback()
	{
		if (IconsFont != -2) return IconsFont;
		IconsFont = -1;

		std::vector<uint8_t> Data;
		if (TwinkUiState::UiMgr and ReadFile(TwinkUiState::UiMgr->DocumentsFolderPath / L"TwinkieX\\Fonts\\ManiaIcons.ttf", Data))
		{
			IconsFont = AddFont("ManiaIcons", std::move(Data));
		}
		return IconsFont;
	}

	static int NvgFont(int Font)
	{
		return Font >= 0 and Font < (int)NvgFontIds.size() ? NvgFontIds[Font] : -1;
	}

	static void SyncFonts(NVGcontext* Ctx)
	{
		if (FontsGeneration != TwinkNvg::Generation)
		{
			FontsGeneration = TwinkNvg::Generation;
			NvgFontIds.clear();
		}

		size_t First = NvgFontIds.size();
		if (First == Fonts.size()) return;

		for (size_t Idx = First; Idx < Fonts.size(); Idx++)
		{
			FontEntry& Entry = *Fonts[Idx];
			NvgFontIds.push_back(Entry.Data.empty() ? -1 : nvgCreateFontMem(Ctx, Entry.Name.c_str(), Entry.Data.data(), (int)Entry.Data.size(), 0));
		}

		for (size_t Idx = 0; Idx < Fonts.size(); Idx++)
		{
			for (int Fallback : Fonts[Idx]->Fallbacks)
			{
				if (Idx < First and Fallback < (int)First) continue;
				int Base = NvgFont((int)Idx), With = NvgFont(Fallback);
				if (Base >= 0 and With >= 0) nvgAddFallbackFontId(Ctx, Base, With);
			}
		}
	}

	static NVGcontext* Vg()
	{
		if (not TwinkNvg::IsInFrame() or not TheScriptManager->IsExecutingRenderCallback()) return nullptr;
		SyncFonts(TwinkNvg::Context);
		return TwinkNvg::Context;
	}

	static NVGcolor Color(const vec4& C) { return nvgRGBAf(C.x, C.y, C.z, C.w); }

	class NvgTexture
	{
		std::vector<uint8_t> Data;
		int Flags = 0;
		int Image = 0;
		uint32_t ImageGeneration = 0;
		mutable size_t RefCount = 1;

	public:
		NvgTexture(std::vector<uint8_t> pData, int pFlags) : Data(std::move(pData)), Flags(pFlags) {}

		~NvgTexture()
		{
			if (Image and TwinkNvg::Context and ImageGeneration == TwinkNvg::Generation) nvgDeleteImage(TwinkNvg::Context, Image);
		}

		void AddRef() { RefCount++; }
		void RemoveRef() { if (!--RefCount) delete this; }

		int Get()
		{
			if (not TwinkNvg::Context) return 0;
			if (Image and ImageGeneration == TwinkNvg::Generation) return Image;

			Image = nvgCreateImageMem(TwinkNvg::Context, Flags, Data.data(), (int)Data.size());
			ImageGeneration = TwinkNvg::Generation;
			return Image;
		}
	};

	static NvgTexture* LoadTextureFile(const std::string& Filename, int Flags)
	{
		std::vector<uint8_t> Data;
		std::filesystem::path Relative = std::filesystem::path((const char8_t*)Filename.c_str()).lexically_normal();
		std::string Key;
		if (not Gecko::ReadScriptFile(Relative, Data, &Key) or Key.empty() or Data.empty())
		{
			Warn(std::format("Texture \"{}\" was not found", Filename));
			return nullptr;
		}
		return new NvgTexture(std::move(Data), Flags);
	}

	static NvgTexture* LoadTextureBuffer(MemoryBuffer& Buffer, int Flags)
	{
		if (!Buffer.GetSize()) return nullptr;

		uint64_t Position = Buffer.GetPosition();
		Buffer.Seek(0);
		std::u8string Bytes = Buffer.ReadString(Buffer.GetSize());
		Buffer.Seek(Position);

		return new NvgTexture(std::vector<uint8_t>(Bytes.begin(), Bytes.end()), Flags);
	}

	static vec2 TextureGetSize(NvgTexture* Self)
	{
		int Image = Self->Get();
		int W = 0, H = 0;
		if (Image) nvgImageSize(TwinkNvg::Context, Image, &W, &H);
		return { (float)W, (float)H };
	}

	static int LoadFont(const std::string& Filename, bool, bool)
	{
		std::vector<uint8_t> Data;
		if (not ReadScriptOrFontsFile(Filename, Data) or Data.empty())
		{
			Warn(std::format("Font \"{}\" was not found", Filename));
			return -1;
		}

		int Font = AddFont(Filename, std::move(Data));
		int Icons = IconsFallback();
		if (Icons >= 0) Fonts[Font]->Fallbacks.push_back(Icons);
		return Font;
	}

	static void AddFallbackFont(int Base, int Fallback)
	{
		if (Base < 0 or Base >= (int)Fonts.size() or Fallback < 0 or Fallback >= (int)Fonts.size()) return;
		Fonts[Base]->Fallbacks.push_back(Fallback);

		if (auto Ctx = TwinkNvg::Context; Ctx and FontsGeneration == TwinkNvg::Generation)
		{
			int B = NvgFont(Base), F = NvgFont(Fallback);
			if (B >= 0 and F >= 0) nvgAddFallbackFontId(Ctx, B, F);
		}
	}

	static void Save() { if (auto Ctx = Vg()) nvgSave(Ctx); }
	static void Restore() { if (auto Ctx = Vg()) nvgRestore(Ctx); }
	static void Reset() { if (auto Ctx = Vg()) nvgReset(Ctx); }
	static void ShapeAntiAlias(bool Enabled) { if (auto Ctx = Vg()) nvgShapeAntiAlias(Ctx, Enabled); }
	static void StrokeColor(const vec4& C) { if (auto Ctx = Vg()) nvgStrokeColor(Ctx, Color(C)); }
	static void StrokePaint(const NVGpaint& Paint) { if (auto Ctx = Vg()) nvgStrokePaint(Ctx, Paint); }
	static void FillColor(const vec4& C) { if (auto Ctx = Vg()) nvgFillColor(Ctx, Color(C)); }
	static void FillPaint(const NVGpaint& Paint) { if (auto Ctx = Vg()) nvgFillPaint(Ctx, Paint); }
	static void MiterLimit(float Limit) { if (auto Ctx = Vg()) nvgMiterLimit(Ctx, Limit); }
	static void StrokeWidth(float Size) { if (auto Ctx = Vg()) nvgStrokeWidth(Ctx, Size); }
	static void LineCap(int Cap) { if (auto Ctx = Vg()) nvgLineCap(Ctx, Cap); }
	static void LineJoin(int Join) { if (auto Ctx = Vg()) nvgLineJoin(Ctx, Join); }
	static void GlobalAlpha(float Alpha) { if (auto Ctx = Vg()) nvgGlobalAlpha(Ctx, Alpha); }

	static void ResetTransform() { if (auto Ctx = Vg()) nvgResetTransform(Ctx); }

	static void SetTransform(const mat3& T)
	{
		if (auto Ctx = Vg())
		{
			nvgResetTransform(Ctx);
			nvgTransform(Ctx, T.x.x, T.x.y, T.y.x, T.y.y, T.z.x, T.z.y);
		}
	}

	static mat3 CurrentTransform()
	{
		float X[6] = { 1, 0, 0, 1, 0, 0 };
		if (auto Ctx = Vg()) nvgCurrentTransform(Ctx, X);
		return { { X[0], X[1], 0 }, { X[2], X[3], 0 }, { X[4], X[5], 1 } };
	}

	static void Transform(const mat3& T) { if (auto Ctx = Vg()) nvgTransform(Ctx, T.x.x, T.x.y, T.y.x, T.y.y, T.z.x, T.z.y); }
	static void Translate(float X, float Y) { if (auto Ctx = Vg()) nvgTranslate(Ctx, X, Y); }
	static void TranslateV(const vec2& V) { Translate(V.x, V.y); }
	static void Rotate(float Angle) { if (auto Ctx = Vg()) nvgRotate(Ctx, Angle); }
	static void SkewX(float Angle) { if (auto Ctx = Vg()) nvgSkewX(Ctx, Angle); }
	static void SkewY(float Angle) { if (auto Ctx = Vg()) nvgSkewY(Ctx, Angle); }
	static void Scale(float X, float Y) { if (auto Ctx = Vg()) nvgScale(Ctx, X, Y); }
	static void ScaleV(const vec2& V) { Scale(V.x, V.y); }

	static NVGpaint LinearGradient(const vec2& Start, const vec2& End, const vec4& C1, const vec4& C2)
	{
		auto Ctx = Vg();
		return Ctx ? nvgLinearGradient(Ctx, Start.x, Start.y, End.x, End.y, Color(C1), Color(C2)) : NVGpaint{};
	}

	static NVGpaint BoxGradient(float X, float Y, float W, float H, float R, float F, const vec4& C1, const vec4& C2)
	{
		auto Ctx = Vg();
		return Ctx ? nvgBoxGradient(Ctx, X, Y, W, H, R, F, Color(C1), Color(C2)) : NVGpaint{};
	}

	static NVGpaint BoxGradientV(const vec2& Pos, const vec2& Size, float R, float F, const vec4& C1, const vec4& C2)
	{
		return BoxGradient(Pos.x, Pos.y, Size.x, Size.y, R, F, C1, C2);
	}

	static NVGpaint RadialGradient(const vec2& Center, float Inner, float Outer, const vec4& C1, const vec4& C2)
	{
		auto Ctx = Vg();
		return Ctx ? nvgRadialGradient(Ctx, Center.x, Center.y, Inner, Outer, Color(C1), Color(C2)) : NVGpaint{};
	}

	static NVGpaint TexturePattern(const vec2& Origin, const vec2& Size, float Angle, NvgTexture* Texture, float Alpha)
	{
		auto Ctx = Vg();
		int Image = Ctx and Texture ? Texture->Get() : 0;
		return Ctx ? nvgImagePattern(Ctx, Origin.x, Origin.y, Size.x, Size.y, Angle, Image, Alpha) : NVGpaint{};
	}

	static void Scissor(float X, float Y, float W, float H) { if (auto Ctx = Vg()) nvgScissor(Ctx, X, Y, W, H); }
	static void IntersectScissor(float X, float Y, float W, float H) { if (auto Ctx = Vg()) nvgIntersectScissor(Ctx, X, Y, W, H); }
	static void ResetScissor() { if (auto Ctx = Vg()) nvgResetScissor(Ctx); }

	static void FontFace(int Font) { if (auto Ctx = Vg()) nvgFontFaceId(Ctx, NvgFont(Font)); }
	static void FontSize(float Size) { if (auto Ctx = Vg()) nvgFontSize(Ctx, Size); }
	static void FontBlur(float Blur) { if (auto Ctx = Vg()) nvgFontBlur(Ctx, Blur); }
	static void TextLetterSpacing(float Spacing) { if (auto Ctx = Vg()) nvgTextLetterSpacing(Ctx, Spacing); }
	static void TextLineHeight(float LineHeight) { if (auto Ctx = Vg()) nvgTextLineHeight(Ctx, LineHeight); }
	static void TextAlign(int Align) { if (auto Ctx = Vg()) nvgTextAlign(Ctx, Align); }

	static float Text(float X, float Y, const std::string& Str)
	{
		auto Ctx = Vg();
		return Ctx ? nvgText(Ctx, X, Y, Str.data(), Str.data() + Str.size()) : X;
	}

	static float TextV(const vec2& Pos, const std::string& Str) { return Text(Pos.x, Pos.y, Str); }

	static void TextBox(float X, float Y, float W, const std::string& Str)
	{
		if (auto Ctx = Vg()) nvgTextBox(Ctx, X, Y, W, Str.data(), Str.data() + Str.size());
	}

	static void TextBoxV(const vec2& Pos, float W, const std::string& Str) { TextBox(Pos.x, Pos.y, W, Str); }

	static vec2 TextBounds(const std::string& Str)
	{
		float B[4] = {};
		if (auto Ctx = Vg()) nvgTextBounds(Ctx, 0, 0, Str.data(), Str.data() + Str.size(), B);
		return { B[2] - B[0], B[3] - B[1] };
	}

	static vec2 TextBoxBounds(float W, const std::string& Str)
	{
		float B[4] = {};
		if (auto Ctx = Vg()) nvgTextBoxBounds(Ctx, 0, 0, W, Str.data(), Str.data() + Str.size(), B);
		return { B[2] - B[0], B[3] - B[1] };
	}

	static void BeginPath() { if (auto Ctx = Vg()) nvgBeginPath(Ctx); }
	static void MoveTo(const vec2& P) { if (auto Ctx = Vg()) nvgMoveTo(Ctx, P.x, P.y); }
	static void LineTo(const vec2& P) { if (auto Ctx = Vg()) nvgLineTo(Ctx, P.x, P.y); }
	static void BezierTo(const vec2& C1, const vec2& C2, const vec2& P) { if (auto Ctx = Vg()) nvgBezierTo(Ctx, C1.x, C1.y, C2.x, C2.y, P.x, P.y); }
	static void QuadTo(const vec2& C, const vec2& P) { if (auto Ctx = Vg()) nvgQuadTo(Ctx, C.x, C.y, P.x, P.y); }
	static void ArcTo(const vec2& P1, const vec2& P2, float R) { if (auto Ctx = Vg()) nvgArcTo(Ctx, P1.x, P1.y, P2.x, P2.y, R); }
	static void ClosePath() { if (auto Ctx = Vg()) nvgClosePath(Ctx); }
	static void PathWinding(int Dir) { if (auto Ctx = Vg()) nvgPathWinding(Ctx, Dir); }
	static void Arc(const vec2& C, float R, float A0, float A1, int Dir) { if (auto Ctx = Vg()) nvgArc(Ctx, C.x, C.y, R, A0, A1, Dir); }
	static void Rect(float X, float Y, float W, float H) { if (auto Ctx = Vg()) nvgRect(Ctx, X, Y, W, H); }
	static void RectV(const vec2& P, const vec2& S) { Rect(P.x, P.y, S.x, S.y); }
	static void RoundedRect(float X, float Y, float W, float H, float R) { if (auto Ctx = Vg()) nvgRoundedRect(Ctx, X, Y, W, H, R); }
	static void RoundedRectV(const vec2& P, const vec2& S, float R) { RoundedRect(P.x, P.y, S.x, S.y, R); }

	static void RoundedRectVarying(float X, float Y, float W, float H, float TL, float TR, float BR, float BL)
	{
		if (auto Ctx = Vg()) nvgRoundedRectVarying(Ctx, X, Y, W, H, TL, TR, BR, BL);
	}

	static void RoundedRectVaryingV(const vec2& P, const vec2& S, float TL, float TR, float BR, float BL)
	{
		RoundedRectVarying(P.x, P.y, S.x, S.y, TL, TR, BR, BL);
	}

	static void Ellipse(const vec2& C, float RX, float RY) { if (auto Ctx = Vg()) nvgEllipse(Ctx, C.x, C.y, RX, RY); }
	static void Circle(const vec2& C, float R) { if (auto Ctx = Vg()) nvgCircle(Ctx, C.x, C.y, R); }
	static void Fill() { if (auto Ctx = Vg()) nvgFill(Ctx); }
	static void Stroke() { if (auto Ctx = Vg()) nvgStroke(Ctx); }

	static void PaintConstruct(NVGpaint* Self) { new (Self) NVGpaint{}; }
	static void PaintConstructCopy(const NVGpaint& Other, NVGpaint* Self) { new (Self) NVGpaint(Other); }
	static NVGpaint& PaintAssign(const NVGpaint& Other, NVGpaint* Self) { return *Self = Other; }

	/*
	!CompatMeta
	for     nvg
	with    Texture@ LoadTexture(const string&in filename, int flags = 0)
	with    Texture@ LoadTexture(MemoryBuffer&in buffer, int flags = 0)
	with    int LoadFont(const string&in filename, bool fallbackIcons = false, bool fallbackArial = false)
	with    void AddFallbackFont(int baseFont, int fallbackFont)
	with    void Save()
	with    void Restore()
	with    void Reset()
	with    void ShapeAntiAlias(bool enabled)
	with    void StrokeColor(const vec4&in color)
	with    void StrokePaint(const Paint&in paint)
	with    void FillColor(const vec4&in color)
	with    void FillPaint(const Paint&in paint)
	with    void MiterLimit(float limit)
	with    void StrokeWidth(float size)
	with    void LineCap(LineCapType cap)
	with    void LineJoin(LineCapType join)
	with    void GlobalAlpha(float alpha)
	with    void ResetTransform()
	with    void SetTransform(const mat3&in t)
	with    mat3 CurrentTransform()
	with    void Transform(const mat3&in t)
	with    void Translate(float x, float y)
	with    void Translate(const vec2&in)
	with    void Rotate(float angle)
	with    void SkewX(float angle)
	with    void SkewY(float angle)
	with    void Scale(float x, float y)
	with    void Scale(const vec2&in)
	with    Paint LinearGradient(const vec2&in start, const vec2&in end, const vec4&in color1, const vec4&in color2)
	with    Paint BoxGradient(float x, float y, float w, float h, float r, float f, const vec4&in color1, const vec4&in color2)
	with    Paint BoxGradient(const vec2&in pos, const vec2&in size, float r, float f, const vec4&in color1, const vec4&in color2)
	with    Paint RadialGradient(const vec2&in center, float inr, float outr, const vec4&in color1, const vec4&in color2)
	with    Paint TexturePattern(const vec2&in origin, const vec2&in size, float angle, Texture@ texture, float alpha)
	with    void Scissor(float x, float y, float w, float h)
	with    void IntersectScissor(float x, float y, float w, float h)
	with    void ResetScissor()
	with    void FontFace(int font)
	with    void FontSize(float size)
	with    void FontBlur(float blur)
	with    void TextLetterSpacing(float spacing)
	with    void TextLineHeight(float lineHeight)
	with    void TextAlign(int align)
	with    float Text(float x, float y, const string&in str)
	with    float Text(const vec2&in pos, const string&in str)
	with    void TextBox(float x, float y, float w, const string&in str)
	with    void TextBox(const vec2&in pos, float w, const string&in str)
	with    vec2 TextBounds(const string&in str)
	with    vec2 TextBoxBounds(float w, const string&in str)
	with    void BeginPath()
	with    void MoveTo(const vec2&in pos)
	with    void LineTo(const vec2&in pos)
	with    void BezierTo(const vec2&in c1, const vec2&in c2, const vec2&in pos)
	with    void QuadTo(const vec2&in c, const vec2&in pos)
	with    void ArcTo(const vec2&in pos1, const vec2&in pos2, float radius)
	with    void ClosePath()
	with    void PathWinding(Winding dir)
	with    void Arc(const vec2&in center, float r, float a0, float a1, Winding dir)
	with    void Rect(float x, float y, float w, float h)
	with    void Rect(const vec2&in pos, const vec2&in size)
	with    void RoundedRect(float x, float y, float w, float h, float r)
	with    void RoundedRect(const vec2&in pos, const vec2&in size, float r)
	with    void RoundedRectVarying(float x, float y, float w, float h, float rtl, float rtr, float rbr, float rbl)
	with    void RoundedRectVarying(const vec2&in pos, const vec2&in size, float rtl, float rtr, float rbr, float rbl)
	with    void Ellipse(const vec2&in center, float rx, float ry)
	with    void Circle(const vec2&in center, float r)
	with    void Fill()
	with    void Stroke()
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	NVG_FN("Texture@ LoadTexture(const string&in filename, int flags = 0)", LoadTextureFile);
	NVG_FN("Texture@ LoadTexture(MemoryBuffer&in buffer, int flags = 0)", LoadTextureBuffer);
	NVG_FN("int LoadFont(const string&in filename, bool fallbackIcons = false, bool fallbackArial = false)", LoadFont);
	NVG_FN("void AddFallbackFont(int baseFont, int fallbackFont)", AddFallbackFont);
	NVG_FN("void Save()", Save);
	NVG_FN("void Restore()", Restore);
	NVG_FN("void Reset()", Reset);
	NVG_FN("void ShapeAntiAlias(bool enabled)", ShapeAntiAlias);
	NVG_FN("void StrokeColor(const vec4&in color)", StrokeColor);
	NVG_FN("void StrokePaint(const Paint&in paint)", StrokePaint);
	NVG_FN("void FillColor(const vec4&in color)", FillColor);
	NVG_FN("void FillPaint(const Paint&in paint)", FillPaint);
	NVG_FN("void MiterLimit(float limit)", MiterLimit);
	NVG_FN("void StrokeWidth(float size)", StrokeWidth);
	NVG_FN("void LineCap(LineCapType cap)", LineCap);
	NVG_FN("void LineJoin(LineCapType join)", LineJoin);
	NVG_FN("void GlobalAlpha(float alpha)", GlobalAlpha);
	NVG_FN("void ResetTransform()", ResetTransform);
	NVG_FN("void SetTransform(const mat3&in t)", SetTransform);
	NVG_FN("mat3 CurrentTransform()", CurrentTransform);
	NVG_FN("void Transform(const mat3&in t)", Transform);
	NVG_FN("void Translate(float x, float y)", Translate);
	NVG_FN("void Translate(const vec2&in)", TranslateV);
	NVG_FN("void Rotate(float angle)", Rotate);
	NVG_FN("void SkewX(float angle)", SkewX);
	NVG_FN("void SkewY(float angle)", SkewY);
	NVG_FN("void Scale(float x, float y)", Scale);
	NVG_FN("void Scale(const vec2&in)", ScaleV);
	NVG_FN("Paint LinearGradient(const vec2&in start, const vec2&in end, const vec4&in color1, const vec4&in color2)", LinearGradient);
	NVG_FN("Paint BoxGradient(float x, float y, float w, float h, float r, float f, const vec4&in color1, const vec4&in color2)", BoxGradient);
	NVG_FN("Paint BoxGradient(const vec2&in pos, const vec2&in size, float r, float f, const vec4&in color1, const vec4&in color2)", BoxGradientV);
	NVG_FN("Paint RadialGradient(const vec2&in center, float inr, float outr, const vec4&in color1, const vec4&in color2)", RadialGradient);
	NVG_FN("Paint TexturePattern(const vec2&in origin, const vec2&in size, float angle, Texture@+ texture, float alpha)", TexturePattern);
	NVG_FN("void Scissor(float x, float y, float w, float h)", Scissor);
	NVG_FN("void IntersectScissor(float x, float y, float w, float h)", IntersectScissor);
	NVG_FN("void ResetScissor()", ResetScissor);
	NVG_FN("void FontFace(int font)", FontFace);
	NVG_FN("void FontSize(float size)", FontSize);
	NVG_FN("void FontBlur(float blur)", FontBlur);
	NVG_FN("void TextLetterSpacing(float spacing)", TextLetterSpacing);
	NVG_FN("void TextLineHeight(float lineHeight)", TextLineHeight);
	NVG_FN("void TextAlign(int align)", TextAlign);
	NVG_FN("float Text(float x, float y, const string&in str)", Text);
	NVG_FN("float Text(const vec2&in pos, const string&in str)", TextV);
	NVG_FN("void TextBox(float x, float y, float w, const string&in str)", TextBox);
	NVG_FN("void TextBox(const vec2&in pos, float w, const string&in str)", TextBoxV);
	NVG_FN("vec2 TextBounds(const string&in str)", TextBounds);
	NVG_FN("vec2 TextBoxBounds(float w, const string&in str)", TextBoxBounds);
	NVG_FN("void BeginPath()", BeginPath);
	NVG_FN("void MoveTo(const vec2&in pos)", MoveTo);
	NVG_FN("void LineTo(const vec2&in pos)", LineTo);
	NVG_FN("void BezierTo(const vec2&in c1, const vec2&in c2, const vec2&in pos)", BezierTo);
	NVG_FN("void QuadTo(const vec2&in c, const vec2&in pos)", QuadTo);
	NVG_FN("void ArcTo(const vec2&in pos1, const vec2&in pos2, float radius)", ArcTo);
	NVG_FN("void ClosePath()", ClosePath);
	NVG_FN("void PathWinding(Winding dir)", PathWinding);
	NVG_FN("void Arc(const vec2&in center, float r, float a0, float a1, Winding dir)", Arc);
	NVG_FN("void Rect(float x, float y, float w, float h)", Rect);
	NVG_FN("void Rect(const vec2&in pos, const vec2&in size)", RectV);
	NVG_FN("void RoundedRect(float x, float y, float w, float h, float r)", RoundedRect);
	NVG_FN("void RoundedRect(const vec2&in pos, const vec2&in size, float r)", RoundedRectV);
	NVG_FN("void RoundedRectVarying(float x, float y, float w, float h, float rtl, float rtr, float rbr, float rbl)", RoundedRectVarying);
	NVG_FN("void RoundedRectVarying(const vec2&in pos, const vec2&in size, float rtl, float rtr, float rbr, float rbl)", RoundedRectVaryingV);
	NVG_FN("void Ellipse(const vec2&in center, float rx, float ry)", Ellipse);
	NVG_FN("void Circle(const vec2&in center, float r)", Circle);
	NVG_FN("void Fill()", Fill);
	NVG_FN("void Stroke()", Stroke);

	/*
	!CompatMeta
	for     nvg
	with    vec2 GetSize()
	being   method
	of      Texture
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	GECKO_EXPORT_METHOD(s_Exports, "nvg", "Texture", "vec2 GetSize()", asFUNCTION(TextureGetSize), asCALL_CDECL_OBJLAST);

	/*
	!CompatMeta
	for     nvg
	with    Paint()
	with    Paint(const nvg::Paint&in)
	being   behavior
	of      Paint
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	GECKO_EXPORT_BEHAVIOUR(s_Exports, "nvg", "Paint", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(PaintConstruct), asCALL_CDECL_OBJLAST);
	GECKO_EXPORT_BEHAVIOUR(s_Exports, "nvg", "Paint", asBEHAVE_CONSTRUCT, "void f(const nvg::Paint&in)", asFUNCTION(PaintConstructCopy), asCALL_CDECL_OBJLAST);
	GECKO_EXPORT_METHOD(s_Exports, "nvg", "Paint", "nvg::Paint& opAssign(const nvg::Paint&in)", asFUNCTION(PaintAssign), asCALL_CDECL_OBJLAST);

	static void RegisterEnum(asIScriptEngine* Engine, const char* Name, std::initializer_list<std::pair<const char*, int>> Values)
	{
		Engine->RegisterEnum(Name);
		for (auto& [ValueName, Value] : Values) Engine->RegisterEnumValue(Name, ValueName, Value);
	}

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("nvg");

		RegisterEnum(Engine, "TextureFlags", {
			{ "None", 0 },
			{ "GenerateMipmaps", NVG_IMAGE_GENERATE_MIPMAPS },
			{ "RepeatX", NVG_IMAGE_REPEATX },
			{ "RepeatY", NVG_IMAGE_REPEATY },
			{ "FlipY", NVG_IMAGE_FLIPY },
			{ "Premultiplied", NVG_IMAGE_PREMULTIPLIED },
			{ "Nearest", NVG_IMAGE_NEAREST },
		});
		RegisterEnum(Engine, "LineCapType", {
			{ "Butt", NVG_BUTT },
			{ "Round", NVG_ROUND },
			{ "Square", NVG_SQUARE },
			{ "Bevel", NVG_BEVEL },
			{ "Miter", NVG_MITER },
		});
		RegisterEnum(Engine, "Winding", {
			{ "CCW", NVG_CCW },
			{ "CW", NVG_CW },
		});
		RegisterEnum(Engine, "Align", {
			{ "Left", NVG_ALIGN_LEFT },
			{ "Center", NVG_ALIGN_CENTER },
			{ "Right", NVG_ALIGN_RIGHT },
			{ "Top", NVG_ALIGN_TOP },
			{ "Middle", NVG_ALIGN_MIDDLE },
			{ "Bottom", NVG_ALIGN_BOTTOM },
			{ "Baseline", NVG_ALIGN_BASELINE },
		});

		Engine->RegisterObjectType("Texture", 0, asOBJ_REF);
		Engine->RegisterObjectBehaviour("Texture", asBEHAVE_ADDREF, "void f()", asMETHOD(NvgTexture, AddRef), asCALL_THISCALL);
		Engine->RegisterObjectBehaviour("Texture", asBEHAVE_RELEASE, "void f()", asMETHOD(NvgTexture, RemoveRef), asCALL_THISCALL);
		Engine->RegisterObjectType("Paint", sizeof(NVGpaint), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<NVGpaint>());

		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
