#pragma once
#include "pch.h"
#include "ILoggert.h"
#include "TwinkLoggert/TwinkLoggert.h"
#include "AngelscriptLoggert/AngelscriptLoggert.h"
#include "CppLoggert/CppLoggert.h"

class TwinkLogs
{
	TwinkLogs operator=(TwinkLogs) = delete;
	TwinkLogs& operator=(TwinkLogs&) = delete;
	TwinkLogs&& operator=(TwinkLogs&&) = delete;

	friend class TwinkUi;
	friend class Twinkie;

protected:
	std::stringstream LogPreview;
	std::fstream LogFile;

	std::vector<ILoggert*> Loggerts;

private:
	void Clear();
	void PullFromLoggerts();
	void Render();
	void UpdateLogFileLoc(std::filesystem::path& Loc);

public:
	// Whether the Logs window is open. The logs are still written to the log file while it's closed.
	bool Visible = true;

	TwinkLogs();
	~TwinkLogs();

	void push_back(ILoggert* Loggert);
	void GetPreview(std::string& Result);
};