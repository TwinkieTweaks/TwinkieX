#include "pch.h"
#include <Twinkie/TwinkLogs/ILoggert.h>

void ILoggert::trace(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)Icon.c_str());
	std::string sTag((const char*)Tag.c_str());

	std::string MessageFormatted = std::format("$f0f[d] {}[{}{}] $g{}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::print(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)Icon.c_str());
	std::string sTag((const char*)Tag.c_str());

	std::string MessageFormatted = std::format("$35e[i] {}[{}{}] $g{}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::warn(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)Icon.c_str());
	std::string sTag((const char*)Tag.c_str());

	std::string MessageFormatted = std::format("$ff0[w] {}[{}{}] $ff0{}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::error(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)Icon.c_str());
	std::string sTag((const char*)Tag.c_str());

	std::string MessageFormatted = std::format("$f00[e] {}[{}{}] $f00{}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}