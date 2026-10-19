#include "pch.h"
#include <Twinkie/TwinkLogs/ILoggert.h>

#define ILoggert_MakeFn(name, color, tag) void ILoggert::##name(std::u8string str) \
{ \
    std::string sStr((const char*)str.c_str()); \
	std::string sColorTag((const char*)ColorTag.c_str()); \
	std::string sIcon((const char*)Icon.c_str()); \
	std::string sTag((const char*)Tag.c_str()); \
	\
    std::string MessageFormattedPrev = std::format(color tag " {}[{}{}] $g{}\n", sColorTag, sIcon, sTag, sStr); \
	std::string MessageFormattedFile = std::format(tag " {}[{}{}] {}\n", sColorTag, sIcon, sTag, sStr); \
	\
	LoggertStreamForPreview << MessageFormattedPrev; \
	LoggertStreamForFile << MessageFormattedFile; \
}

ILoggert_MakeFn(trace, "$f0f", "[d]")
ILoggert_MakeFn(print, "$35e", "[i]")
ILoggert_MakeFn(warn,  "$ff0", "[w]")
ILoggert_MakeFn(error, "$f00", "[e]")