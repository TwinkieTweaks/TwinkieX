#pragma once
#include "pch.h"
#include <memory>
#include <optional>

namespace Gecko
{
	class Script;

	enum class SettingType : int32_t
	{
		Unknown,
		Bool,
		Enum,
		Float,
		Double,
		Int8,
		Int16,
		Int32,
		String,
		Vec2,
		Vec3,
		Vec4,
		Uint8,
		Uint16,
		Uint32,
		Int2,
		Int3,
		Nat2,
		Nat3,
		Quat,
	};

	struct SettingsSection
	{
		std::string Name;

		bool Get(const std::string& Key, std::string& Out) const;
		void Set(const std::string& Key, const std::string& Value) const;
		void Remove(const std::string& Key) const;
		void Clear() const;
		std::vector<std::string> Keys() const;
	};

	struct SettingCondition
	{
		asIScriptFunction* Function = nullptr;
		void* Variable = nullptr;
		std::optional<int32_t> EnumValue;
		bool Negate = false;
	};

	struct PluginSetting
	{
		Script* Owner = nullptr;

		std::string Namespace;
		std::string VarName;
		std::string Name;
		std::string Description;
		std::string Category;
		std::vector<std::string> Aliases;

		SettingType Type = SettingType::Unknown;
		int TypeId = 0;
		std::string TypeName;
		void* Address = nullptr;
		asITypeInfo* EnumType = nullptr;

		bool Hidden = false;
		bool Drag = false;
		bool Color = false;
		bool Multiline = false;
		bool Password = false;
		std::optional<double> Min;
		std::optional<double> Max;
		std::optional<double> Step;

		std::optional<SettingCondition> If;
		std::optional<SettingCondition> EnableIf;
		asIScriptFunction* OnChange = nullptr;
		asIScriptFunction* BeforeRender = nullptr;
		asIScriptFunction* AfterRender = nullptr;

		std::vector<uint8_t> Default;
		std::string DefaultString;

		std::string Key() const;
		std::string ToString() const;
		bool FromString(std::string_view Text);
		bool IsDefault() const;
		void Reset();
	};

	struct SettingsTab
	{
		asIScriptFunction* Function = nullptr;
		std::string Name = "Script";
		std::string Icon;
		int32_t Order = 0;
	};

	void CollectSettings(Script& Owner, asIScriptModule* Module, CScriptBuilder& Builder);
	void RegisterSettings(Script& Owner, const SettingsSection& Section);
	void UnregisterSettings(const SettingsSection& Section);

	bool EvaluateCondition(PluginSetting& Setting, const SettingCondition& Condition);
	bool RenderSetting(PluginSetting& Setting);
	void NotifySettingChanged(PluginSetting& Setting);

	bool ParseSettingBool(std::string_view Text, bool& Out);
	std::string EscapeSettingString(std::string_view Text);
	std::string UnescapeSettingString(std::string_view Text);
	std::string SettingEnumValueName(asITypeInfo* EnumType, int32_t Value);
}
