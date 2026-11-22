#include "pch.h"
#include "ExportTime.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <cctype>
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

namespace Gecko::Exports::Time
{
	static std::vector<GeckoExport> s_Exports;
#define TIME_FN(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "Time", Decl_, Fn_)

	struct Info
	{
		int Year, Month, Day, Hour, Minute, Second;
	};

	using namespace std::chrono;

	static const steady_clock::time_point s_Start = steady_clock::now();

	static uint64_t Now() { return (uint64_t)duration_cast<milliseconds>(steady_clock::now() - s_Start).count(); }
	static int64_t Stamp() { return (int64_t)duration_cast<seconds>(system_clock::now().time_since_epoch()).count(); }
	static int64_t MilliStamp() { return (int64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); }
	static int64_t MicroStamp() { return (int64_t)duration_cast<microseconds>(system_clock::now().time_since_epoch()).count(); }
	static uint64_t FrameCount() { return ::ImGui::GetCurrentContext() ? (uint64_t)::ImGui::GetFrameCount() : 0; }

	static bool ToTm(int64_t Stamp, bool Utc, std::tm& Out)
	{
		std::time_t T = Stamp < 0 ? std::time(nullptr) : (std::time_t)Stamp;
		return (Utc ? gmtime_s(&Out, &T) : localtime_s(&Out, &T)) == 0;
	}

	static std::string SafeFormat(const std::string& Format)
	{
		static const std::string_view Known = "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%";
		std::string Out;
		for (size_t Idx = 0; Idx < Format.size(); Idx++)
		{
			if (Format[Idx] != '%')
			{
				Out += Format[Idx];
				continue;
			}

			size_t Spec = Idx + 1;
			if (Spec < Format.size() and (Format[Spec] == '#' or Format[Spec] == 'E' or Format[Spec] == 'O')) Spec++;
			if (Spec < Format.size() and Known.find(Format[Spec]) != std::string_view::npos)
			{
				Out.append(Format, Idx, Spec - Idx + 1);
				Idx = Spec;
			}
			else
			{
				Out += "%%";
			}
		}
		return Out;
	}

	static std::string FormatTm(const std::string& Format, int64_t Stamp, bool Utc)
	{
		std::tm Tm{};
		if (Format.empty() or not ToTm(Stamp, Utc, Tm)) return {};

		std::string Safe = SafeFormat(Format);
		std::string Out(Safe.size() * 4 + 64, '\0');
		for (int Try = 0; Try < 4; Try++, Out.resize(Out.size() * 4))
		{
			if (size_t Written = std::strftime(Out.data(), Out.size(), Safe.c_str(), &Tm))
			{
				Out.resize(Written);
				return Out;
			}
		}
		return {};
	}

	static std::string FormatString(const std::string& Format, int64_t Stamp) { return FormatTm(Format, Stamp, false); }
	static std::string FormatStringUTC(const std::string& Format, int64_t Stamp) { return FormatTm(Format, Stamp, true); }

	static int64_t ParseFormatString(const std::string& Format, const std::string& Stamp)
	{
		std::tm Tm{};
		std::istringstream Stream(Stamp);
		Stream >> std::get_time(&Tm, SafeFormat(Format).c_str());
		if (Stream.fail()) return 0;

		Tm.tm_isdst = -1;
		std::time_t T = std::mktime(&Tm);
		return T == (std::time_t)-1 ? 0 : (int64_t)T;
	}

	static std::string FormatSigned(int64_t Time, bool Fractions, bool ForceMinutes, bool ForceHours, bool Short)
	{
		(void)Short;

		std::string Out = Time < 0 ? "-" : "";
		uint64_t Abs = Time < 0 ? (uint64_t)0 - (uint64_t)Time : (uint64_t)Time;

		uint64_t Hours = Abs / 3600000, Minutes = Abs / 60000 % 60, Seconds = Abs / 1000 % 60, Millis = Abs % 1000;

		if (Hours > 0 or ForceHours)
		{
			Out += std::format("{}:{:02}:{:02}", Hours, Minutes, Seconds);
		}
		else if (Minutes > 0 or ForceMinutes)
		{
			Out += std::format("{}:{:02}", Minutes, Seconds);
		}
		else
		{
			Out += std::format("{}", Seconds);
		}

		if (Fractions) Out += std::format(".{:03}", Millis);
		return Out;
	}

	static std::string FormatUnsigned(uint64_t Time, bool Fractions, bool ForceMinutes, bool ForceHours, bool Short)
	{
		if (Time > (uint64_t)INT64_MAX) Time = (uint64_t)INT64_MAX;
		return FormatSigned((int64_t)Time, Fractions, ForceMinutes, ForceHours, Short);
	}

	static uint64_t ParseRelativeTime(const std::string& Str)
	{
		std::string_view Text = Str;
		while (not Text.empty() and std::isspace((unsigned char)Text.front())) Text.remove_prefix(1);
		while (not Text.empty() and std::isspace((unsigned char)Text.back())) Text.remove_suffix(1);
		if (Text.empty()) return 0;

		uint64_t Millis = 0;
		size_t Dot = Text.find('.');
		if (Dot != std::string_view::npos)
		{
			std::string_view Fraction = Text.substr(Dot + 1);
			if (Fraction.empty() or Fraction.size() > 3) return 0;
			uint64_t Scale = 100;
			for (char C : Fraction)
			{
				if (not std::isdigit((unsigned char)C)) return 0;
				Millis += (uint64_t)(C - '0') * Scale;
				Scale /= 10;
			}
			Text = Text.substr(0, Dot);
		}

		uint64_t Seconds = 0;
		int Parts = 0;
		while (true)
		{
			size_t Colon = Text.find(':');
			std::string_view Part = Text.substr(0, Colon);
			if (Part.empty() or ++Parts > 3) return 0;

			uint64_t Value = 0;
			for (char C : Part)
			{
				if (not std::isdigit((unsigned char)C)) return 0;
				Value = Value * 10 + (uint64_t)(C - '0');
			}
			Seconds = Seconds * 60 + Value;

			if (Colon == std::string_view::npos) break;
			Text = Text.substr(Colon + 1);
		}

		return Seconds * 1000 + Millis;
	}

	static Info ParseTm(int64_t Stamp, bool Utc)
	{
		std::tm Tm{};
		if (not ToTm(Stamp, Utc, Tm)) return {};
		return { Tm.tm_year + 1900, Tm.tm_mon + 1, Tm.tm_mday, Tm.tm_hour, Tm.tm_min, Tm.tm_sec };
	}

	static Info Parse(int64_t Stamp) { return ParseTm(Stamp, false); }
	static Info ParseUTC(int64_t Stamp) { return ParseTm(Stamp, true); }

	/*
	!CompatMeta
	for     Time
	with    uint64 get_Now()
	with    int64 get_Stamp()
	with    int64 get_MilliStamp()
	with    int64 get_MicroStamp()
	with    uint64 get_FrameCount()
	with    string FormatString(const string&in format, int64 stamp = -1)
	with    string FormatStringUTC(const string&in format, int64 stamp = -1)
	with    int64 ParseFormatString(const string&in format, const string&in stamp)
	with    uint64 ParseRelativeTime(const string&in time)
	with    Info Parse(int64 stamp = -1)
	with    Info ParseUTC(int64 stamp = -1)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	TIME_FN("uint64 get_Now() property", Now);
	TIME_FN("int64 get_Stamp() property", Stamp);
	TIME_FN("int64 get_MilliStamp() property", MilliStamp);
	TIME_FN("int64 get_MicroStamp() property", MicroStamp);
	TIME_FN("uint64 get_FrameCount() property", FrameCount);
	TIME_FN("string FormatString(const string&in format, int64 stamp = -1)", FormatString);
	TIME_FN("string FormatStringUTC(const string&in format, int64 stamp = -1)", FormatStringUTC);
	TIME_FN("int64 ParseFormatString(const string&in format, const string&in stamp)", ParseFormatString);
	TIME_FN("uint64 ParseRelativeTime(const string&in time)", ParseRelativeTime);
	TIME_FN("Info Parse(int64 stamp = -1)", Parse);
	TIME_FN("Info ParseUTC(int64 stamp = -1)", ParseUTC);

	/*
	!CompatMeta
	for     Time
	with    string Format(uint64 time, bool fractions = true, bool forceMinutes = true, bool forceHours = false, bool short = false)
	with    string Format(int64 time, bool fractions = true, bool forceMinutes = true, bool forceHours = false, bool short = false)
	being   function
	declare partial
	reason  short is ignored
	since   v0.1.1
	!CompatMeta
	*/
	TIME_FN("string Format(uint64 time, bool fractions = true, bool forceMinutes = true, bool forceHours = false, bool short = false)", FormatUnsigned);
	TIME_FN("string Format(int64 time, bool fractions = true, bool forceMinutes = true, bool forceHours = false, bool short = false)", FormatSigned);

	/*
	!CompatMeta
	for     Time
	with    int Year
	with    int Month
	with    int Day
	with    int Hour
	with    int Minute
	with    int Second
	being   prop
	of      Info
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Year", offsetof(Info, Year));
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Month", offsetof(Info, Month));
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Day", offsetof(Info, Day));
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Hour", offsetof(Info, Hour));
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Minute", offsetof(Info, Minute));
	GECKO_EXPORT_PROP(s_Exports, "Time", "Info", "int Second", offsetof(Info, Second));

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("Time");
		Engine->RegisterObjectType("Info", sizeof(Info), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_ALLINTS | asGetTypeTraits<Info>());
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
