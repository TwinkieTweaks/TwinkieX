#include "pch.h"
#include "ExportString.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <cctype>

namespace Gecko::Exports::String
{
	static std::vector<GeckoExport> s_Exports;
#define STRING_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "", "string", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	static int IndexOrMinusOne(size_t Index)
	{
		return Index == std::string::npos ? -1 : (int)Index;
	}

	static std::string Lowered(std::string Str)
	{
		for (char& C : Str) C = (char)std::tolower((unsigned char)C);
		return Str;
	}

	static bool StartsWith(const std::string& Prefix, const std::string& Self) { return Self.starts_with(Prefix); }
	static bool EndsWith(const std::string& Suffix, const std::string& Self) { return Self.ends_with(Suffix); }
	static bool Contains(const std::string& Part, const std::string& Self) { return Self.find(Part) != std::string::npos; }
	static int IndexOf(const std::string& Part, const std::string& Self) { return IndexOrMinusOne(Self.find(Part)); }
	static int IndexOfI(const std::string& Part, const std::string& Self) { return IndexOrMinusOne(Lowered(Self).find(Lowered(Part))); }
	static int LastIndexOf(const std::string& Part, const std::string& Self) { return IndexOrMinusOne(Self.rfind(Part)); }
	static int Length(const std::string& Self) { return (int)Self.size(); }

	static std::string Trim(const std::string& Self)
	{
		const char* Whitespace = " \t\r\n\v\f";
		size_t Begin = Self.find_first_not_of(Whitespace);
		if (Begin == std::string::npos) return {};
		return Self.substr(Begin, Self.find_last_not_of(Whitespace) - Begin + 1);
	}

	static std::string ToLower(const std::string& Self) { return Lowered(Self); }

	static std::string ToUpper(const std::string& Self)
	{
		std::string Out = Self;
		for (char& C : Out) C = (char)std::toupper((unsigned char)C);
		return Out;
	}

	static std::string SubStrLength(int Index, int Count, const std::string& Self)
	{
		size_t Begin = (size_t)std::clamp(Index, 0, (int)Self.size());
		return Self.substr(Begin, Count < 0 ? std::string::npos : (size_t)Count);
	}

	static std::string SubStr(int Index, const std::string& Self)
	{
		return SubStrLength(Index, -1, Self);
	}

	static std::string Replace(const std::string& Search, const std::string& With, const std::string& Self)
	{
		if (Search.empty()) return Self;

		std::string Out;
		size_t From = 0;
		for (size_t Found; (Found = Self.find(Search, From)) != std::string::npos; From = Found + Search.size())
		{
			Out.append(Self, From, Found - From);
			Out += With;
		}
		Out.append(Self, From, std::string::npos);
		return Out;
	}

	static CScriptArray* Split(const std::string& Delimiter, int Limit, const std::string& Self)
	{
		asIScriptContext* Ctx = asGetActiveContext();
		CScriptArray* Parts = CScriptArray::Create(Ctx->GetEngine()->GetTypeInfoByDecl("array<string>"));

		auto Add = [&](std::string Part) { Parts->Resize(Parts->GetSize() + 1); *(std::string*)Parts->At(Parts->GetSize() - 1) = std::move(Part); };

		size_t From = 0;
		if (not Delimiter.empty())
		{
			for (size_t Found; (Limit <= 0 or (int)Parts->GetSize() < Limit - 1) and (Found = Self.find(Delimiter, From)) != std::string::npos; From = Found + Delimiter.size())
			{
				Add(Self.substr(From, Found - From));
			}
		}
		Add(Self.substr(From));
		return Parts;
	}

	/*
	!CompatMeta
	for     Global
	with    bool StartsWith(const string&in) const
	with    bool EndsWith(const string&in) const
	with    bool Contains(const string&in) const
	with    int LastIndexOf(const string&in) const
	with    int IndexOf(const string&in) const
	with    int IndexOfI(const string&in) const
	with    int get_Length() const
	with    string Trim() const
	with    string ToLower() const
	with    string ToUpper() const
	with    string SubStr(int index) const
	with    string SubStr(int index, int length) const
	with    string Replace(const string&in search, const string&in replace) const
	with    string[]@ Split(const string&in str, int limit = 0) const
	being   method
	of      string
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	STRING_METHOD("bool StartsWith(const string&in) const", StartsWith);
	STRING_METHOD("bool EndsWith(const string&in) const", EndsWith);
	STRING_METHOD("bool Contains(const string&in) const", Contains);
	STRING_METHOD("int LastIndexOf(const string&in) const", LastIndexOf);
	STRING_METHOD("int IndexOf(const string&in) const", IndexOf);
	STRING_METHOD("int IndexOfI(const string&in) const", IndexOfI);
	STRING_METHOD("int get_Length() const property", Length);
	STRING_METHOD("string Trim() const", Trim);
	STRING_METHOD("string ToLower() const", ToLower);
	STRING_METHOD("string ToUpper() const", ToUpper);
	STRING_METHOD("string SubStr(int index) const", SubStr);
	STRING_METHOD("string SubStr(int index, int length) const", SubStrLength);
	STRING_METHOD("string Replace(const string&in search, const string&in replace) const", Replace);
	STRING_METHOD("string[]@ Split(const string&in str, int limit = 0) const", Split);

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterAll(Engine, s_Exports);
	}
}
