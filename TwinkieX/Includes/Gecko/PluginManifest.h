#pragma once
#include "pch.h"
#include <optional>

namespace Gecko
{
	struct PluginManifest
	{
		bool Present = false;

		std::string Name;
		std::string Author;
		std::string Category = "Uncategorized";
		std::string Version = "1.0";
		std::vector<std::string> Blocks;
		int64_t SiteId = 0;
		bool Essential = false;

		std::string MinGameVersion;
		std::string MaxGameVersion;
		std::vector<std::string> SupportedGames;

		std::optional<uint32_t> TimeoutMs;
		std::vector<std::string> Imports;
		std::vector<std::string> Exports;
		std::vector<std::string> SharedExports;
		std::vector<std::string> Dependencies;
		std::vector<std::string> ExportDependencies;
		std::vector<std::string> OptionalDependencies;
		std::vector<std::string> Defines;
		std::string Module;
		bool ControlsOtherPlugins = false;
	};

	bool ParsePluginManifest(std::string_view Source, PluginManifest& Manifest, std::string& Error);
}
