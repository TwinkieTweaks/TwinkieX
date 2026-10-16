#include "pch.h"
#include "Twinkie/TwinkLogs/TwinkLogs.h"

TwinkLogs::TwinkLogs() 
{
	push_back(&LoggertTwink);
	push_back(&LoggertCpp);
	push_back(&LoggertAngelscript);
}

TwinkLogs::~TwinkLogs()
{
	// TODO: Save logs
}

void TwinkLogs::push_back(ILoggert* Loggert)
{
	Loggerts.push_back(Loggert);
}

void TwinkLogs::GetPreview(std::string& Result)
{
	LogPreview.seekp(0);
	Result = LogPreview.str();
}

void TwinkLogs::Clear()
{
	LogPreview.clear();
}

void TwinkLogs::PullFromLoggerts()
{
	for (auto& Loggert : Loggerts)
	{
		LogPreview << Loggert->LoggertStreamForPreview.str();
		LogFile << Loggert->LoggertStreamForFile.str();

		Loggert->LoggertStreamForPreview.clear();
		Loggert->LoggertStreamForFile.clear();
	}
}

void TwinkLogs::Render()
{
	using namespace ImGui;

	Begin("Logs##Twinkie");

	auto LastPos = LogPreview.tellp();
	LogPreview.seekp(0);

	std::string Line;
	while (std::getline(LogPreview, Line))
	{
		Text(Line.c_str());
	}

	LogPreview.seekp(LastPos);

	End();
}