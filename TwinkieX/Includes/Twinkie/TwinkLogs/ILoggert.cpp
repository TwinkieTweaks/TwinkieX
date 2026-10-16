#include "pch.h"
#include <Twinkie/TwinkLogs/ILoggert.h>

void ILoggert::trace(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)ColorTag.c_str());
	std::string sTag((const char*)ColorTag.c_str());

	std::string MessageFormatted = std::format("[debug] {}[{}{}]$g {}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::print(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)ColorTag.c_str());
	std::string sTag((const char*)ColorTag.c_str());

	std::string MessageFormatted = std::format("[info]  {}[{}{}]$g {}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::warn(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)ColorTag.c_str());
	std::string sTag((const char*)ColorTag.c_str());

	std::string MessageFormatted = std::format("[warn]  {}[{}{}]$g {}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}

void ILoggert::error(std::u8string str)
{
	// TODO: Timestamp
	std::string sStr((const char*)str.c_str());
	std::string sColorTag((const char*)ColorTag.c_str());
	std::string sIcon((const char*)ColorTag.c_str());
	std::string sTag((const char*)ColorTag.c_str());

	std::string MessageFormatted = std::format("[error] {}[{}{}]$g {}\n", sColorTag, sIcon, sTag, sStr);

	LoggertStreamForPreview << MessageFormatted;
	LoggertStreamForFile << MessageFormatted;
}