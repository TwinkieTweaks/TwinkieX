#include "pch.h"
#include "ExportText.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/Exports/Math/Math.h>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>

namespace Gecko::Exports::Text
{
	static std::vector<GeckoExport> s_Exports;
#define TEXT_FN(Fn_, Decl_) GECKO_EXPORT_FN(s_Exports, "Text", Decl_, Fn_)
#define TEXT_FN_EX(Fn_, Decl_) GECKO_EXPORT_FN_EX(s_Exports, "Text", Decl_, Fn_, asCALL_CDECL)


	template<typename T>
	requires std::floating_point<T>
	static T stot(const std::string& s)
	{
		const auto* Begin = s.data();
		const auto* End = s.data() + s.size();

		T Value{};

		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Value, std::chars_format::general);
		if (ErrorCode != std::errc{} || Ptr != End)
		{
			return T{};
		}
		return Value;
	}

	template <typename T>
	static T stot(const std::string& s, int Base = 10) 
	{
		const auto* Begin = s.data();
		const auto* End = s.data() + s.size();

		T Value{};

		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Value, Base);
		if (ErrorCode != std::errc{} || Ptr != End) 
		{
			return T{};
		}
		return Value;
	}

	template<typename T>
	requires std::floating_point<T>
	static bool stottry(const std::string& s, T& Value)
	{
		if (s.empty()) return false;

		const auto* Begin = s.data();
		const auto* End = s.data() + s.size();

		T Parsed{};
		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Parsed, std::chars_format::general);
		if (ErrorCode != std::errc{} || Ptr != End)
		{
			Value = T{};
			return false;
		}
		Value = Parsed;
		return true;
	}

	template <typename T>
	static bool stottry(const std::string& s, T& Value, int Base = 10)
	{
		if (s.empty()) return false;
		const auto* Begin = s.data();
		const auto* End = s.data() + s.size();

		T Parsed{};
		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Parsed, Base);
		if (ErrorCode != std::errc{} || Ptr != End)
		{
			Value = T{};
			return false;
		}

		Value = Parsed;
		return true;
	}

	/*
	!CompatMeta
	for     Text
	with    int ParseInt(const string&in str, int base = 10)
	as      int ParseInt(const string&in str, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stot<int>, "int ParseInt(const string&in str, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    int64 ParseInt64(const string&in str, int base = 10)
	as      int64 ParseInt64(const string&in str, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stot<int64_t>, "int64 ParseInt64(const string&in str, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    uint ParseUInt(const string&in str, int base = 10)
	as      uint ParseUInt(const string&in str, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stot<unsigned int>, "uint ParseUInt(const string&in str, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    uint64 ParseUInt64(const string&in str, int base = 10)
	as      uint64 ParseUInt64(const string&in str, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stot<uint64_t>, "uint64 ParseUInt64(const string&in str, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    float ParseFloat(const string&in str)
	as      float ParseFloat(const string&in str)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN_EX(asFUNCTIONPR(stot<float>, (const std::string&), float), "float ParseFloat(const string&in str)");

	/*
	!CompatMeta
	for     Text
	with    double ParseDouble(const string&in str)
	as      double ParseDouble(const string&in str)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN_EX(asFUNCTIONPR(stot<double>, (const std::string&), double), "double ParseDouble(const string&in str)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseInt(const string&in str, int&out, int base = 10)
	as      bool TryParseInt(const string&in str, int&out, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stottry<int>, "bool TryParseInt(const string&in str, int&out, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseInt64(const string&in str, int64&out, int base = 10)
	as      bool TryParseInt64(const string&in str, int64&out, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stottry<int64_t>, "bool TryParseInt64(const string&in str, int64&out, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseUInt(const string&in str, int&out, int base = 10)
	as      bool TryParseUInt(const string&in str, uint&out, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stottry<unsigned int>, "bool TryParseUInt(const string&in str, uint&out, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseUInt64(const string&in str, int64&out, int base = 10)
	as      bool TryParseUInt64(const string&in str, uint64&out, int base = 10)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(stottry<uint64_t>, "bool TryParseUInt64(const string&in str, uint64&out, int base = 10)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseFloat(const string&in str, float&out)
	as      bool TryParseFloat(const string&in str, float&out)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN_EX(asFUNCTIONPR(stottry<float>, (const std::string&, float&), bool), "bool TryParseFloat(const string&in str, float&out)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseDouble(const string&in str, double&out)
	as      bool TryParseDouble(const string&in str, double&out)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN_EX(asFUNCTIONPR(stottry<double>, (const std::string&, double&), bool), "bool TryParseDouble(const string&in str, double&out)");

	// Script errors

	static void SetScriptException(const char* Message)
	{
		if (auto Ctx = asGetActiveContext()) Ctx->SetException(Message);
	}

	// Hex colors

	static int HexDigit(char C)
	{
		if (C >= '0' and C <= '9') return C - '0';
		if (C >= 'a' and C <= 'f') return C - 'a' + 10;
		if (C >= 'A' and C <= 'F') return C - 'A' + 10;
		return -1;
	}

	static bool HexColorFrom(const std::string& Str, Math::vec4& Out)
	{
		std::string_view Hex = Str;
		if (not Hex.empty() and Hex[0] == '#') Hex.remove_prefix(1);

		size_t Len = Hex.size();
		if (Len != 3 and Len != 4 and Len != 6 and Len != 8) return false;

		size_t DigitsPerChannel = Len <= 4 ? 1 : 2;
		float Channels[4] = { 1, 1, 1, 1 };
		for (size_t Channel = 0; Channel < Len / DigitsPerChannel; Channel++)
		{
			int Value = 0;
			for (size_t Digit = 0; Digit < DigitsPerChannel; Digit++)
			{
				int D = HexDigit(Hex[Channel * DigitsPerChannel + Digit]);
				if (D < 0) return false;
				Value = Value * 16 + D;
			}
			Channels[Channel] = Value / (DigitsPerChannel == 1 ? 15.0f : 255.0f);
		}

		Out = { Channels[0], Channels[1], Channels[2], Channels[3] };
		return true;
	}

	/*
	!CompatMeta
	for     Text
	with    vec4 ParseHexColor(string str)
	as      vec4 ParseHexColor(const string&in str)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static Math::vec4 ParseHexColor(const std::string& Str)
	{
		Math::vec4 Color = {};
		HexColorFrom(Str, Color);
		return Color;
	}
	TEXT_FN(ParseHexColor, "vec4 ParseHexColor(const string&in str)");

	/*
	!CompatMeta
	for     Text
	with    bool TryParseHexColor(string str, vec4&out)
	as      bool TryParseHexColor(const string&in str, vec4&out)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static bool TryParseHexColor(const std::string& Str, Math::vec4& Out)
	{
		Out = {};
		return HexColorFrom(Str, Out);
	}
	TEXT_FN(TryParseHexColor, "bool TryParseHexColor(const string&in str, vec4&out)");

	// Format

	template<typename T>
	static std::string Format(const std::string& Fmt, T Value)
	{
		std::string Out;
		bool Converted = false;

		for (size_t i = 0; i < Fmt.size(); i++)
		{
			if (Fmt[i] != '%')
			{
				Out += Fmt[i];
				continue;
			}
			if (i + 1 < Fmt.size() and Fmt[i + 1] == '%')
			{
				Out += '%';
				i++;
				continue;
			}

			// %[flags][width][.precision][length]conversion
			size_t j = i + 1;
			std::string Spec = "%";
			auto TakeDigits = [&]
			{
				size_t Start = j;
				while (j < Fmt.size() and std::isdigit((unsigned char)Fmt[j])) Spec += Fmt[j++];
				return j - Start <= 3;
			};

			while (j < Fmt.size() and std::strchr("-+ #0", Fmt[j]) and Fmt[j] != '\0') Spec += Fmt[j++];
			bool SaneSize = TakeDigits();
			if (j < Fmt.size() and Fmt[j] == '.')
			{
				Spec += Fmt[j++];
				SaneSize = TakeDigits() and SaneSize;
			}

			while (j < Fmt.size() and Fmt[j] != '\0' and std::strchr("hlLqjzt", Fmt[j])) j++;

			if (j >= Fmt.size() or not SaneSize)
			{
				SetScriptException("Invalid format");
				return {};
			}
			if (Converted)
			{
				SetScriptException("Format can only have one value in it, such as %d or %.2f");
				return {};
			}

			char Conversion = Fmt[j];
			int Len = -1;
			std::string Buffer(64, '\0');
			auto Print = [&](auto Arg)
			{
				Len = std::snprintf(Buffer.data(), Buffer.size(), Spec.c_str(), Arg);
				if (Len >= (int)Buffer.size())
				{
					Buffer.resize(Len + 1);
					Len = std::snprintf(Buffer.data(), Buffer.size(), Spec.c_str(), Arg);
				}
			};

			if (Conversion and std::strchr("di", Conversion))
			{
				Spec += "ll";
				Spec += Conversion;
				Print((long long)Value);
			}
			else if (Conversion and std::strchr("ouxX", Conversion))
			{
				Spec += "ll";
				Spec += Conversion;
				Print(std::is_signed_v<T> ? (unsigned long long)(long long)Value : (unsigned long long)Value);
			}
			else if (Conversion == 'c')
			{
				Spec += 'c';
				Print((int)Value);
			}
			else if (Conversion and std::strchr("fFeEgGaA", Conversion))
			{
				Spec += Conversion;
				Print((double)Value);
			}
			else
			{
				SetScriptException("Format only supports d, i, o, u, x, X, c, f, F, e, E, g, G, a and A");
				return {};
			}

			if (Len < 0)
			{
				SetScriptException("Invalid format");
				return {};
			}

			Out.append(Buffer.data(), Len);
			Converted = true;
			i = j;
		}

		return Out;
	}

	/*
	!CompatMeta
	for     Text
	with    string Format(const string&in format, int8)
	with    string Format(const string&in format, int16)
	with    string Format(const string&in format, int)
	with    string Format(const string&in format, int64)
	with    string Format(const string&in format, uint8)
	with    string Format(const string&in format, uint16)
	with    string Format(const string&in format, uint)
	with    string Format(const string&in format, uint64)
	with    string Format(const string&in format, float)
	with    string Format(const string&in format, double)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TEXT_FN(Format<int8_t>, "string Format(const string&in format, int8)");
	TEXT_FN(Format<int16_t>, "string Format(const string&in format, int16)");
	TEXT_FN(Format<int32_t>, "string Format(const string&in format, int)");
	TEXT_FN(Format<int64_t>, "string Format(const string&in format, int64)");
	TEXT_FN(Format<uint8_t>, "string Format(const string&in format, uint8)");
	TEXT_FN(Format<uint16_t>, "string Format(const string&in format, uint16)");
	TEXT_FN(Format<uint32_t>, "string Format(const string&in format, uint)");
	TEXT_FN(Format<uint64_t>, "string Format(const string&in format, uint64)");
	TEXT_FN(Format<float>, "string Format(const string&in format, float)");
	TEXT_FN(Format<double>, "string Format(const string&in format, double)");

	/*
	!CompatMeta
	for     Text
	with    string FormatPointer(uint64 ptr)
	as      string FormatPointer(uint64 ptr)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string FormatPointer(uint64_t Ptr)
	{
		return std::format("0x{:X}", Ptr);
	}
	TEXT_FN(FormatPointer, "string FormatPointer(uint64 ptr)");

	static std::string ColorDigits(const Math::vec3& Rgb)
	{
		std::string Out;
		for (float Channel : { Rgb.x, Rgb.y, Rgb.z })
		{
			if (not (Channel >= 0.0f)) Channel = 0.0f;
			if (Channel > 1.0f) Channel = 1.0f;
			Out += "0123456789abcdef"[(int)std::lround(Channel * 15.0f)];
		}
		return Out;
	}

	/*
	!CompatMeta
	for     Text
	with    string FormatGameColor(const vec3&in rgb)
	as      string FormatGameColor(const vec3&in rgb)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string FormatGameColor(const Math::vec3& Rgb)
	{
		return "$" + ColorDigits(Rgb);
	}
	TEXT_FN(FormatGameColor, "string FormatGameColor(const vec3&in rgb)");

	/*
	!CompatMeta
	for     Text
	with    string FormatOpenplanetColor(const vec3&in rgb)
	as      string FormatOpenplanetColor(const vec3&in rgb)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string FormatOpenplanetColor(const Math::vec3& Rgb)
	{
		return "\\$" + ColorDigits(Rgb);
	}
	TEXT_FN(FormatOpenplanetColor, "string FormatOpenplanetColor(const vec3&in rgb)");

	// Formatting codes

	enum class PartKind
	{
		Text,
		Dollar,
		Color,
		Code,
	};

	template<typename F>
	static void ForEachPart(const std::string& Str, bool Openplanet, F&& OnPart)
	{
		const size_t Len = Str.size();
		size_t i = 0;
		while (i < Len)
		{
			size_t CodeStart = std::string::npos;
			if (not Openplanet and Str[i] == '$') CodeStart = i + 1;
			else if (Openplanet and Str[i] == '\\' and i + 1 < Len and Str[i + 1] == '$') CodeStart = i + 2;

			if (CodeStart == std::string::npos)
			{
				size_t TextEnd = i + 1;
				while (TextEnd < Len and Str[TextEnd] != (Openplanet ? '\\' : '$')) TextEnd++;
				OnPart(PartKind::Text, std::string_view(Str).substr(i, TextEnd - i));
				i = TextEnd;
				continue;
			}

			if (CodeStart >= Len) break;

			char Code = Str[CodeStart];
			if (Code == '$')
			{
				OnPart(PartKind::Dollar, std::string_view("$"));
				i = CodeStart + 1;
			}
			else if (HexDigit(Code) >= 0)
			{
				size_t End = CodeStart + 1;
				if (Openplanet) End = (std::min)(CodeStart + 3, Len);
				else while (End < Len and End < CodeStart + 3 and HexDigit(Str[End]) >= 0) End++;

				OnPart(PartKind::Color, std::string_view(Str).substr(CodeStart, End - CodeStart));
				i = End;
			}
			else
			{
				size_t End = CodeStart + 1;

				// "$l[https://...]text$l"
				char Lower = (char)std::tolower((unsigned char)Code);
				if (not Openplanet and (Lower == 'l' or Lower == 'h' or Lower == 'p') and End < Len and Str[End] == '[')
				{
					size_t Close = Str.find(']', End);
					End = Close == std::string::npos ? Len : Close + 1;
				}

				OnPart(PartKind::Code, std::string_view(Str).substr(CodeStart, End - CodeStart));
				i = End;
			}
		}
	}

	/*
	!CompatMeta
	for     Text
	with    string StripFormatCodes(const string&in s)
	as      string StripFormatCodes(const string&in s)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string StripFormatCodes(const std::string& Str)
	{
		std::string Out;
		ForEachPart(Str, false, [&](PartKind Kind, std::string_view Part)
		{
			if (Kind == PartKind::Text or Kind == PartKind::Dollar) Out += Part;
		});
		return Out;
	}
	TEXT_FN(StripFormatCodes, "string StripFormatCodes(const string&in s)");

	/*
	!CompatMeta
	for     Text
	with    string StripNonColorFormatCodes(const string&in s)
	as      string StripNonColorFormatCodes(const string&in s)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string StripNonColorFormatCodes(const std::string& Str)
	{
		std::string Out;
		ForEachPart(Str, false, [&](PartKind Kind, std::string_view Part)
		{
			switch (Kind)
			{
			case PartKind::Text: Out += Part; break;
			case PartKind::Dollar: Out += "$$"; break;
			case PartKind::Color: Out += '$'; Out += Part; break;
			case PartKind::Code:
				if (Part.size() == 1 and std::tolower((unsigned char)Part[0]) == 'g') Out += "$g";
				break;
			}
		});
		return Out;
	}
	TEXT_FN(StripNonColorFormatCodes, "string StripNonColorFormatCodes(const string&in s)");

	/*
	!CompatMeta
	for     Text
	with    string OpenplanetFormatCodes(const string&in s)
	as      string OpenplanetFormatCodes(const string&in s)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string OpenplanetFormatCodes(const std::string& Str)
	{
		std::string Out;
		ForEachPart(Str, false, [&](PartKind Kind, std::string_view Part)
		{
			switch (Kind)
			{
			case PartKind::Text: Out += Part; break;
			case PartKind::Dollar: Out += "\\$$"; break;
			case PartKind::Color:
				Out += "\\$";
				Out += Part;
				Out.append(3 - Part.size(), Part.back());
				break;
			case PartKind::Code:
			{
				char Code = (char)std::tolower((unsigned char)Part[0]);
				if (Part.size() == 1 and std::strchr("oiwnmtsgz", Code) and Code != '\0')
				{
					Out += "\\$";
					Out += Code;
				}
				break;
			}
			}
		});
		return Out;
	}
	TEXT_FN(OpenplanetFormatCodes, "string OpenplanetFormatCodes(const string&in s)");

	/*
	!CompatMeta
	for     Text
	with    string StripOpenplanetFormatCodes(const string&in s)
	as      string StripOpenplanetFormatCodes(const string&in s)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string StripOpenplanetFormatCodes(const std::string& Str)
	{
		std::string Out;
		ForEachPart(Str, true, [&](PartKind Kind, std::string_view Part)
		{
			if (Kind == PartKind::Text or Kind == PartKind::Dollar) Out += Part;
		});
		return Out;
	}
	TEXT_FN(StripOpenplanetFormatCodes, "string StripOpenplanetFormatCodes(const string&in s)");

	// Hex and base64

	/*
	!CompatMeta
	for     Text
	with    string EncodeHex(const string&in, bool upper = false)
	as      string EncodeHex(const string&in, bool upper = false)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string EncodeHex(const std::string& Str, bool Upper)
	{
		const char* Digits = Upper ? "0123456789ABCDEF" : "0123456789abcdef";
		std::string Out;
		Out.reserve(Str.size() * 2);
		for (unsigned char Byte : Str)
		{
			Out += Digits[Byte >> 4];
			Out += Digits[Byte & 0xF];
		}
		return Out;
	}
	TEXT_FN(EncodeHex, "string EncodeHex(const string&in, bool upper = false)");

	/*
	!CompatMeta
	for     Text
	with    string DecodeHex(const string&in)
	as      string DecodeHex(const string&in)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	// Returns an empty string if the input isn't hex
	static std::string DecodeHex(const std::string& Str)
	{
		if (Str.size() % 2 != 0) return {};

		std::string Out;
		Out.reserve(Str.size() / 2);
		for (size_t i = 0; i < Str.size(); i += 2)
		{
			int High = HexDigit(Str[i]), Low = HexDigit(Str[i + 1]);
			if (High < 0 or Low < 0) return {};
			Out += (char)((High << 4) | Low);
		}
		return Out;
	}
	TEXT_FN(DecodeHex, "string DecodeHex(const string&in)");

	static const char* Base64Alphabet(bool Url)
	{
		return Url ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
		           : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	}

	/*
	!CompatMeta
	for     Text
	with    string EncodeBase64(const string&in, bool url = false)
	as      string EncodeBase64(const string&in, bool url = false)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string EncodeBase64(const std::string& Str, bool Url)
	{
		const char* Alphabet = Base64Alphabet(Url);
		std::string Out;
		Out.reserve((Str.size() + 2) / 3 * 4);

		for (size_t i = 0; i < Str.size(); i += 3)
		{
			size_t Count = (std::min<size_t>)(3, Str.size() - i);
			uint32_t Triple = 0;
			for (size_t b = 0; b < 3; b++)
			{
				Triple = (Triple << 8) | (b < Count ? (unsigned char)Str[i + b] : 0);
			}

			for (size_t c = 0; c < 4; c++)
			{
				Out += c <= Count ? Alphabet[(Triple >> (18 - 6 * c)) & 0x3F] : '=';
			}
		}
		return Out;
	}
	TEXT_FN(EncodeBase64, "string EncodeBase64(const string&in, bool url = false)");

	/*
	!CompatMeta
	for     Text
	with    string DecodeBase64(const string&in, bool url = false)
	as      string DecodeBase64(const string&in, bool url = false)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	// Padding is optional. Returns an empty string if the input isn't base64.
	static std::string DecodeBase64(const std::string& Str, bool Url)
	{
		int Values[256];
		std::fill(std::begin(Values), std::end(Values), -1);
		const char* Alphabet = Base64Alphabet(Url);
		for (int i = 0; i < 64; i++) Values[(unsigned char)Alphabet[i]] = i;

		std::string_view Input = Str;
		while (not Input.empty() and Input.back() == '=') Input.remove_suffix(1);
		if (Input.size() % 4 == 1 or Str.size() - Input.size() > 2) return {};

		std::string Out;
		Out.reserve(Input.size() * 3 / 4);

		uint32_t Bits = 0;
		int BitCount = 0;
		for (unsigned char C : Input)
		{
			int Value = Values[C];
			if (Value < 0) return {};

			Bits = (Bits << 6) | (uint32_t)Value;
			BitCount += 6;
			if (BitCount >= 8)
			{
				BitCount -= 8;
				Out += (char)((Bits >> BitCount) & 0xFF);
			}
		}
		return Out;
	}
	TEXT_FN(DecodeBase64, "string DecodeBase64(const string&in, bool url = false)");

	// Arrays and repeats

	/*
	!CompatMeta
	for     Text
	with    string Join(const string[]&in arr, const string&in delimiter)
	as      string Join(const string[]&in arr, const string&in delimiter)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string Join(const CScriptArray& Arr, const std::string& Delimiter)
	{
		std::string Out;
		for (asUINT i = 0; i < Arr.GetSize(); i++)
		{
			if (i) Out += Delimiter;
			Out += *(const std::string*)Arr.At(i);
		}
		return Out;
	}
	TEXT_FN(Join, "string Join(const string[]&in arr, const string&in delimiter)");

	/*
	!CompatMeta
	for     Text
	with    string Repeat(const string&in str, int count)
	as      string Repeat(const string&in str, int count)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	static std::string Repeat(const std::string& Str, int Count)
	{
		if (Count <= 0 or Str.empty()) return {};

		// A script asking for gigabytes gets an error instead of taking the game down
		if ((uint64_t)Str.size() * (uint64_t)Count > 256ull * 1024 * 1024)
		{
			SetScriptException("Repeat's result would be too big");
			return {};
		}

		std::string Out;
		Out.reserve(Str.size() * Count);
		for (int i = 0; i < Count; i++) Out += Str;
		return Out;
	}
	TEXT_FN(Repeat, "string Repeat(const string&in str, int count)");

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterAll(Engine, s_Exports);
	}
}