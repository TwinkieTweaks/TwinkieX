#include "pch.h"
#include "ExportString.h"
#include <Gecko/Exports/GeckoAutoExport.h>

namespace Gecko::Exports::String
{
	static std::vector<GeckoExport> s_Exports;
#define WSTRING_BEHAVIOUR(Beh_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "", "wstring", Beh_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define WSTRING_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "", "wstring", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define STRING_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "", "string", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	std::wstring Utf8ToWide(const std::string& Str)
	{
		if (Str.empty()) return {};
		int Size = MultiByteToWideChar(CP_UTF8, 0, Str.data(), (int)Str.size(), nullptr, 0);
		if (Size <= 0) return {};
		std::wstring Wide((size_t)Size, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, Str.data(), (int)Str.size(), Wide.data(), Size);
		return Wide;
	}

	std::string WideToUtf8(const std::wstring& Str)
	{
		if (Str.empty()) return {};
		int Size = WideCharToMultiByte(CP_UTF8, 0, Str.data(), (int)Str.size(), nullptr, 0, nullptr, nullptr);
		if (Size <= 0) return {};
		std::string Utf8((size_t)Size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, Str.data(), (int)Str.size(), Utf8.data(), Size, nullptr, nullptr);
		return Utf8;
	}

	static void Construct(std::wstring* Self) { new (Self) std::wstring(); }
	static void ConstructCopy(const std::wstring& Other, std::wstring* Self) { new (Self) std::wstring(Other); }
	static void ConstructFromString(const std::string& Other, std::wstring* Self) { new (Self) std::wstring(Utf8ToWide(Other)); }
	static void Destruct(std::wstring* Self) { Self->~basic_string(); }
	static std::wstring& Assign(const std::wstring& Other, std::wstring* Self) { return *Self = Other; }
	static std::string ToString(const std::wstring& Self) { return WideToUtf8(Self); }
	static int Compare(const std::wstring& Other, const std::wstring& Self) { int R = Self.compare(Other); return R < 0 ? -1 : R > 0; }
	static int CompareString(const std::string& Other, const std::wstring& Self) { return Compare(Utf8ToWide(Other), Self); }
	static bool StartsWith(const std::wstring& Prefix, const std::wstring& Self) { return Self.starts_with(Prefix); }
	static bool EndsWith(const std::wstring& Suffix, const std::wstring& Self) { return Self.ends_with(Suffix); }
	static bool Contains(const std::wstring& Part, const std::wstring& Self) { return Self.find(Part) != std::wstring::npos; }
	static int Length(const std::wstring& Self) { return (int)Self.size(); }

	static std::wstring SubStrLength(int Index, int Count, const std::wstring& Self)
	{
		size_t Begin = (size_t)std::clamp(Index, 0, (int)Self.size());
		return Self.substr(Begin, Count < 0 ? std::wstring::npos : (size_t)Count);
	}

	static std::wstring SubStr(int Index, const std::wstring& Self) { return SubStrLength(Index, -1, Self); }

	static void StringFromWide(const std::wstring& Other, std::string* Self) { new (Self) std::string(WideToUtf8(Other)); }
	static std::wstring StringToWide(const std::string& Self) { return Utf8ToWide(Self); }
	static std::string StringAddWide(const std::wstring& Other, const std::string& Self) { return Self + WideToUtf8(Other); }
	static std::string StringAddWideReversed(const std::wstring& Other, const std::string& Self) { return WideToUtf8(Other) + Self; }

	/*
	!CompatMeta
	for     Global
	with    wstring()
	with    wstring(const wstring&in)
	with    wstring(const string&in)
	with    ~wstring()
	being   behavior
	of      wstring
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	WSTRING_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f()", Construct);
	WSTRING_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(const wstring&in)", ConstructCopy);
	WSTRING_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(const string&in)", ConstructFromString);
	WSTRING_BEHAVIOUR(asBEHAVE_DESTRUCT, "void f()", Destruct);

	/*
	!CompatMeta
	for     Global
	with    int opCmp(const wstring&in) const
	with    int opCmp(const string&in) const
	with    wstring& opAssign(const wstring&in)
	with    string opImplConv() const
	with    bool StartsWith(const wstring&in) const
	with    bool EndsWith(const wstring&in) const
	with    bool Contains(const wstring&in) const
	with    wstring SubStr(int index) const
	with    wstring SubStr(int index, int length) const
	with    int get_Length() const
	being   method
	of      wstring
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	WSTRING_METHOD("int opCmp(const wstring&in) const", Compare);
	WSTRING_METHOD("int opCmp(const string&in) const", CompareString);
	WSTRING_METHOD("wstring& opAssign(const wstring&in)", Assign);
	WSTRING_METHOD("string opImplConv() const", ToString);
	WSTRING_METHOD("bool StartsWith(const wstring&in) const", StartsWith);
	WSTRING_METHOD("bool EndsWith(const wstring&in) const", EndsWith);
	WSTRING_METHOD("bool Contains(const wstring&in) const", Contains);
	WSTRING_METHOD("wstring SubStr(int index) const", SubStr);
	WSTRING_METHOD("wstring SubStr(int index, int length) const", SubStrLength);
	WSTRING_METHOD("int get_Length() const property", Length);

	/*
	!CompatMeta
	for     Global
	with    string(const wstring&in)
	being   behavior
	of      string
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	GECKO_EXPORT_BEHAVIOUR(s_Exports, "", "string", asBEHAVE_CONSTRUCT, "void f(const wstring&in)", asFUNCTION(StringFromWide), asCALL_CDECL_OBJLAST);

	/*
	!CompatMeta
	for     Global
	with    string opAdd(const wstring&in) const
	with    string opAdd_r(const wstring&in) const
	with    wstring opImplConv() const
	being   method
	of      string
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	STRING_METHOD("string opAdd(const wstring&in) const", StringAddWide);
	STRING_METHOD("string opAdd_r(const wstring&in) const", StringAddWideReversed);
	STRING_METHOD("wstring opImplConv() const", StringToWide);

	void RegisterWString(asIScriptEngine* Engine)
	{
		Engine->RegisterObjectType("wstring", sizeof(std::wstring), asOBJ_VALUE | asGetTypeTraits<std::wstring>());
		RegisterAll(Engine, s_Exports);
	}
}
