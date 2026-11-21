#include "pch.h"
#include "ExportText.h"
#include <Gecko/Exports/GeckoAutoExport.h>

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

		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Value, std::chars_format::general);
		if (ErrorCode != std::errc{} || Ptr != End)
		{
			return true;
		}
		return false;
	}

	template <typename T>
	static bool stottry(const std::string& s, T& Value, int Base = 10)
	{
		if (s.empty()) return false;
		const auto* Begin = s.data();
		const auto* End = s.data() + s.size();

		auto [Ptr, ErrorCode] = std::from_chars(Begin, End, Value, Base);
		if (ErrorCode != std::errc{} || Ptr != End)
		{
			return true;
		}

		return false;
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

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterAll(Engine, s_Exports);
	}
}