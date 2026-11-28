#pragma once
#include "pch.h"
#include "Awaitable.h"

namespace Gecko
{
	void InstallGameCallbacks();
	void UninstallGameCallbacks();
	bool IsRunContextActive(RunContext);
	void SaveSettingsOnExit(const char* Reason);
}
