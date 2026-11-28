#pragma once
#include "pch.h"
#include <format>
#include <mutex>

// Represents a "logging profile" for an internal system.
// NOTE: Icon (if added) needs an extra space at the end to look good.
// [{Level}] {Color}[{Icon}{Tag}]$g {Message}
class ILoggert
{
	friend class TwinkLogs;

	std::stringstream LoggertStreamForPreview{};
	std::stringstream LoggertStreamForFile{};

	static std::ostream* FileSink;
	static std::mutex& Mutex();

	void Write(const std::string& Preview, const char* Level, const std::string& Message);
public:
#ifdef NDEBUG
	static constexpr bool VerboseEnabled = false;
#else
	static constexpr bool VerboseEnabled = true;
#endif

	ILoggert operator=(ILoggert) = delete;
	ILoggert& operator=(ILoggert&) = delete;
	ILoggert&& operator=(ILoggert&&) = delete;

	// Represented as TrackmaniaFormatting
	std::u8string ColorTag;

	std::u8string Tag;
	std::u8string Icon;

	bool ResetColorForMessage = false;

	ILoggert();
	ILoggert(std::u8string ColorTag, std::u8string Tag, std::u8string Icon, bool ResetColorForMessage = false) : 
		ColorTag(ColorTag), 
		Tag(Tag), 
		Icon(Icon), 
		ResetColorForMessage(ResetColorForMessage) {};

	virtual ~ILoggert() {};

	virtual void trace(std::u8string);
	virtual void print(std::u8string);
	virtual void warn(std::u8string);
	virtual void error(std::u8string);
	virtual void verbose(std::u8string);
};

inline long long LogMsSince(std::chrono::steady_clock::time_point Start)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - Start).count();
}

template<typename... TArgs>
std::u8string LogFormat(std::format_string<TArgs...> Format, TArgs&&... Args)
{
	std::string Text = std::format(Format, std::forward<TArgs>(Args)...);
	return std::u8string(Text.begin(), Text.end());
}