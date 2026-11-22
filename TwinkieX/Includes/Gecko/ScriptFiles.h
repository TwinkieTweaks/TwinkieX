#pragma once
#include "pch.h"
#include "Script.h"

namespace Gecko
{
	Script* CallingScript();

	bool ReadScriptFile(const std::filesystem::path& Relative, std::vector<uint8_t>& Out, std::string* Key = nullptr);
}
