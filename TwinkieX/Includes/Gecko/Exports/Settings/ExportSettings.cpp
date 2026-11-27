#include "pch.h"
#include "ExportSettings.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/PluginSettings.h>
#include <cstdlib>

namespace Gecko::Exports::Settings
{
	static std::vector<GeckoExport> s_Exports;
	static asIScriptEngine* s_Engine = nullptr;

#define SECTION_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Settings", "Section", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	static int32_t Section_GetInt(const std::string& Key, int32_t Default, SettingsSection* Self)
	{
		std::string Value;
		return Self->Get(Key, Value) ? (int32_t)std::strtol(Value.c_str(), nullptr, 10) : Default;
	}

	static float Section_GetFloat(const std::string& Key, float Default, SettingsSection* Self)
	{
		std::string Value;
		return Self->Get(Key, Value) ? std::strtof(Value.c_str(), nullptr) : Default;
	}

	static void Section_Clear(SettingsSection* Self)
	{
		Self->Clear();
	}

	static bool Section_GetBool(const std::string& Key, bool Default, SettingsSection* Self)
	{
		std::string Value;
		bool Result = Default;
		return Self->Get(Key, Value) && ParseSettingBool(Value, Result) ? Result : Default;
	}


	static std::string Section_GetString(const std::string& Key, const std::string& Default, SettingsSection* Self)
	{
		std::string Value;
		return Self->Get(Key, Value) ? UnescapeSettingString(Value) : Default;
	}

	static void Section_SetBool(const std::string& Key, bool Value, SettingsSection* Self)
	{
		Self->Set(Key, Value ? "true" : "false");
	}

	static void Section_SetInt(const std::string& Key, int32_t Value, SettingsSection* Self)
	{
		Self->Set(Key, std::to_string(Value));
	}

	static void Section_SetFloat(const std::string& Key, float Value, SettingsSection* Self)
	{
		Self->Set(Key, std::format("{}", Value));
	}

	static void Section_SetString(const std::string& Key, const std::string& Value, SettingsSection* Self)
	{
		Self->Set(Key, EscapeSettingString(Value));
	}

	static CScriptArray* Section_GetKeys(SettingsSection* Self)
	{
		std::vector<std::string> Keys = Self->Keys();
		CScriptArray* Array = CScriptArray::Create(s_Engine->GetTypeInfoByDecl("array<string>"), (asUINT)Keys.size());
		for (asUINT Idx = 0; Idx < Keys.size(); Idx++) Array->SetValue(Idx, &Keys[Idx]);
		return Array;
	}

	/*
	!CompatMeta
	for     Settings
	with    void Clear()
	with    bool GetBool(const string&in key, bool def = false)
	with    int GetInt(const string&in key, int def = 0)
	with    float GetFloat(const string&in key, float def = 0.0f)
	with    string GetString(const string&in key, const string&in def = "")
	with    void SetBool(const string&in key, bool def = false)
	with    void SetInt(const string&in key, int def = 0)
	with    void SetFloat(const string&in key, float def = 0.0f)
	with    void SetString(const string&in key, const string&in def = "")
	with    string[]@ GetKeys()
	being   method
	of      Section
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	SECTION_METHOD("void Clear()", Section_Clear);
	SECTION_METHOD("bool GetBool(const string&in key, bool def = false)", Section_GetBool);
	SECTION_METHOD("int GetInt(const string&in key, int def = 0)", Section_GetInt);
	SECTION_METHOD("float GetFloat(const string&in key, float def = 0.0f)", Section_GetFloat);
	SECTION_METHOD("string GetString(const string&in key, const string&in def = \"\")", Section_GetString);
	SECTION_METHOD("void SetBool(const string&in key, bool def = false)", Section_SetBool);
	SECTION_METHOD("void SetInt(const string&in key, int def = 0)", Section_SetInt);
	SECTION_METHOD("void SetFloat(const string&in key, float def = 0.0f)", Section_SetFloat);
	SECTION_METHOD("void SetString(const string&in key, const string&in def = \"\")", Section_SetString);
	SECTION_METHOD("string[]@ GetKeys()", Section_GetKeys);

	void Registrar(asIScriptEngine* Engine)
	{
		s_Engine = Engine;

		Engine->SetDefaultNamespace("Settings");
		Engine->RegisterObjectType("Section", 0, asOBJ_REF | asOBJ_NOCOUNT);
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
