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

protected:
	std::stringstream LogPreview;
	std::ofstream LogFile;

	std::vector<ILoggert*> Loggerts;

private:
	void Clear();
	void PullFromLoggerts();
	void Render();

public:
	TwinkLogs();
	~TwinkLogs();

	void push_back(ILoggert* Loggert);
	void GetPreview(std::string& Result);
};