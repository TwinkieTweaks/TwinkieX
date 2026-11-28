#pragma once
#include "pch.h"
#include "CtxSet.h"
#include "Awaitable.h"
#include "PluginManifest.h"
#include "PluginSettings.h"

namespace Gecko
{
	class Script
	{
	public:
		Script operator=(Script) = delete;

		Script(const Script&) = default;
		Script(Script&&) = default;
		Script(Script&) = default;
		Script() = default;

		std::filesystem::path ScriptPath = "";
		std::u8string Id = u8"";
		std::u8string Name = u8"";
		std::u8string DisplayName = u8"";
		PluginManifest Manifest;
		std::string LoadError;
		std::vector<std::u8string> SourceCodes;
		// Section name of each source code, used in compiler messages
		std::vector<std::u8string> SourceNames;
		std::vector<std::u8string> ExportCodes;
		std::vector<std::u8string> ExportNames;
		CtxSet Ctxs{};
		WaitState MainWait;
		std::vector<Coroutine*> Coroutines;
		std::vector<uint32_t> LoadCallbackIds;
		std::vector<std::shared_ptr<PluginSetting>> Settings;
		std::vector<SettingsTab> SettingsTabs;
		SettingsSection SettingsStore;
		bool Enabled = true;
		bool Favorite = false;
	};
}