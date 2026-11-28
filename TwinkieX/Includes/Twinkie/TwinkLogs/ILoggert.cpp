#include "pch.h"
#include <Twinkie/TwinkLogs/ILoggert.h>

std::ostream* ILoggert::FileSink = nullptr;

std::mutex& ILoggert::Mutex()
{
	static std::mutex LogMutex;
	return LogMutex;
}

static std::string LogPrefix()
{
	SYSTEMTIME Time{};
	GetLocalTime(&Time);
	if constexpr (ILoggert::VerboseEnabled)
	{
		return std::format("{:02}:{:02}:{:02}.{:03} T{:<5} ", Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds, GetCurrentThreadId());
	}
	else
	{
		return std::format("{:02}:{:02}:{:02}.{:03} ", Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
	}
}

void ILoggert::Write(const std::string& Preview, const char* Level, const std::string& Message)
{
	std::string Line = std::format("{}{} [{}{}] {}\n", LogPrefix(), Level, (const char*)Icon.c_str(), (const char*)Tag.c_str(), Message);

	std::lock_guard Lock(Mutex());
	LoggertStreamForPreview << Preview;
	if (FileSink)
	{
		*FileSink << Line;
		FileSink->flush();
	}
	else
	{
		LoggertStreamForFile << Line;
	}

	if constexpr (VerboseEnabled)
	{
		fputs(Line.c_str(), stdout);
	}
}

#define ILoggert_MakeFn(name, color, tag) void ILoggert::##name(std::u8string str) \
{ \
	std::string sStr((const char*)str.c_str()); \
	Write(std::format(color tag " {}[{}{}] \\$g{}\n", (const char*)ColorTag.c_str(), (const char*)Icon.c_str(), (const char*)Tag.c_str(), sStr), tag, sStr); \
}

#define ILoggert_MakeFnExtraColored(name, color, tag) void ILoggert::##name(std::u8string str) \
{ \
	std::string sStr((const char*)str.c_str()); \
	Write(std::format(color tag " {}[{}{}] " color "{}\n", (const char*)ColorTag.c_str(), (const char*)Icon.c_str(), (const char*)Tag.c_str(), sStr), tag, sStr); \
}

ILoggert_MakeFnExtraColored(trace, "\\$999", "[d]")
ILoggert_MakeFn(print, "\\$35e", "[i]")
ILoggert_MakeFnExtraColored(warn,  "\\$ff0", "[w]")
ILoggert_MakeFnExtraColored(error, "\\$f00", "[e]")

void ILoggert::verbose(std::u8string str)
{
	if constexpr (VerboseEnabled)
	{
		std::string sStr((const char*)str.c_str());
		Write(std::format("\\$777[v] {}[{}{}] \\$777{}\n", (const char*)ColorTag.c_str(), (const char*)Icon.c_str(), (const char*)Tag.c_str(), sStr), "[v]", sStr);
	}
	else
	{
		(void)str;
	}
}
