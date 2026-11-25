#include "pch.h"
#include "PluginManifest.h"

#pragma warning(push, 0)
#include <tomlplusplus/toml.hpp>
#pragma warning(pop)

namespace Gecko
{
	static void ReadString(const toml::table& Table, const char* Key, std::string& Out)
	{
		const toml::node* Node = Table.get(Key);
		if (std::optional<std::string> Value = Node ? Node->value<std::string>() : std::nullopt) Out = *Value;
	}

	static void ReadStrings(const toml::table& Table, const char* Key, std::vector<std::string>& Out)
	{
		const toml::array* Array = Table.get_as<toml::array>(Key);
		if (!Array) return;

		for (const toml::node& Element : *Array)
		{
			if (std::optional<std::string> Value = Element.value<std::string>()) Out.push_back(*Value);
		}
	}

	static void ReadBool(const toml::table& Table, const char* Key, bool& Out)
	{
		const toml::node* Node = Table.get(Key);
		if (std::optional<bool> Value = Node ? Node->value<bool>() : std::nullopt) Out = *Value;
	}

	bool ParsePluginManifest(std::string_view Source, PluginManifest& Manifest, std::string& Error)
	{
		toml::table Root;
		try
		{
			Root = toml::parse(Source);
		}
		catch (const toml::parse_error& Failure)
		{
			Error = std::format("{} (line {}, column {})", Failure.description(), Failure.source().begin.line, Failure.source().begin.column);
			return false;
		}

		Manifest = {};
		Manifest.Present = true;

		if (const toml::table* Meta = Root.get_as<toml::table>("meta"))
		{
			ReadString(*Meta, "name", Manifest.Name);
			ReadString(*Meta, "author", Manifest.Author);
			ReadString(*Meta, "category", Manifest.Category);
			ReadString(*Meta, "version", Manifest.Version);
			ReadStrings(*Meta, "blocks", Manifest.Blocks);
			ReadBool(*Meta, "essential", Manifest.Essential);
			if (const toml::node* SiteId = Meta->get("siteid")) Manifest.SiteId = SiteId->value<int64_t>().value_or(0);
		}

		if (const toml::table* Game = Root.get_as<toml::table>("game"))
		{
			ReadString(*Game, "min_version", Manifest.MinGameVersion);
			ReadString(*Game, "max_version", Manifest.MaxGameVersion);
			ReadStrings(*Game, "supported_games", Manifest.SupportedGames);
		}

		if (const toml::table* Script = Root.get_as<toml::table>("script"))
		{
			const toml::node* TimeoutNode = Script->get("timeout");
			if (std::optional<int64_t> Timeout = TimeoutNode ? TimeoutNode->value<int64_t>() : std::nullopt)
			{
				Manifest.TimeoutMs = (uint32_t)std::clamp<int64_t>(*Timeout, 0, UINT32_MAX);
			}
			ReadStrings(*Script, "imports", Manifest.Imports);
			ReadStrings(*Script, "exports", Manifest.Exports);
			ReadStrings(*Script, "shared_exports", Manifest.SharedExports);
			ReadStrings(*Script, "dependencies", Manifest.Dependencies);
			ReadStrings(*Script, "export_dependencies", Manifest.ExportDependencies);
			ReadStrings(*Script, "optional_dependencies", Manifest.OptionalDependencies);
			ReadStrings(*Script, "defines", Manifest.Defines);
			ReadString(*Script, "module", Manifest.Module);
			ReadBool(*Script, "controls_other_plugins", Manifest.ControlsOtherPlugins);
		}

		return true;
	}
}
