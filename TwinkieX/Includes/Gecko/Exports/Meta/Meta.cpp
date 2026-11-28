#include "pch.h"
#include "Meta.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/ScriptFiles.h>
#include <Gecko/Exports/Math/Math.h>
#include <Gecko/Exports/Global/Global.h>

namespace Gecko::Exports::Meta
{
	static std::vector<GeckoExport> s_Exports;
	static asIScriptEngine* s_Engine = nullptr;

#define META_FN(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "Meta", Decl_, Fn_)
#define PLUGIN_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Meta", "Plugin", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_BEHAVIOUR(Beh_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "Meta", "UnloadedPluginInfo", Beh_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Meta", "UnloadedPluginInfo", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_PROP(Decl_, Offset_) GECKO_EXPORT_PROP(s_Exports, "Meta", "UnloadedPluginInfo", Decl_, Offset_)
#define SETTING_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Meta", "PluginSetting", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	enum class PluginType : int32_t
	{
		Unknown,
		Legacy,
		Folder,
		Zip,
	};

	enum class PluginSource : int32_t
	{
		Unknown,
		ApplicationFolder,
		UserFolder,
	};

	struct UnloadedPluginInfo
	{
		std::string Path;
		std::string ID;
		PluginSource Source = PluginSource::Unknown;
		PluginType Type = PluginType::Unknown;
	};

	static std::string ToStr(const std::u8string& Str)
	{
		return std::string((const char*)Str.data(), Str.size());
	}

	static CScriptArray* ToStringArray(const std::vector<std::string>& Strings)
	{
		CScriptArray* Array = CScriptArray::Create(s_Engine->GetTypeInfoByDecl("array<string>"), (asUINT)Strings.size());
		for (asUINT Idx = 0; Idx < Strings.size(); Idx++) Array->SetValue(Idx, (void*)&Strings[Idx]);
		return Array;
	}

	static PluginType TypeOf(const Script& Plugin)
	{
		std::error_code Error;
		return std::filesystem::is_directory(Plugin.ScriptPath, Error) ? PluginType::Folder : PluginType::Zip;
	}

	static bool IsLoaded(const Script& Plugin)
	{
		return Plugin.LoadError.empty();
	}

	static void SetPluginEnabled(Script* Self, bool Enabled)
	{
		if (!TheScriptManager) return;
		TheScriptManager->SetEnabled((size_t)(Self - TheScriptManager->Scripts.data()), Enabled);
	}

	static std::string Plugin_GetID(Script* Self) { return ToStr(Self->Id); }
	static PluginType Plugin_GetType(Script* Self) { return TypeOf(*Self); }
	static std::string Plugin_GetName(Script* Self) { return ToStr(Self->DisplayName); }
	static std::string Plugin_GetAuthor(Script* Self) { return Self->Manifest.Author; }
	static std::string Plugin_GetCategory(Script* Self) { return Self->Manifest.Category; }
	static CScriptArray* Plugin_GetBlocks(Script* Self) { return ToStringArray(Self->Manifest.Blocks); }
	static CScriptArray* Plugin_GetDependencies(Script* Self) { return ToStringArray(Self->Manifest.Dependencies); }
	static CScriptArray* Plugin_GetOptionalDependencies(Script* Self) { return ToStringArray(Self->Manifest.OptionalDependencies); }
	static bool Plugin_GetHasManifest(Script* Self) { return Self->Manifest.Present; }
	static int32_t Plugin_GetSignatureLevel(Script*) { return 1; }
	static int32_t Plugin_GetSiteID(Script* Self) { return (int32_t)Self->Manifest.SiteId; }
	static std::string Plugin_GetVersion(Script* Self) { return Self->Manifest.Version; }
	static bool Plugin_GetEssential(Script* Self) { return Self->Manifest.Essential; }
	static PluginSource Plugin_GetSource(Script*) { return PluginSource::UserFolder; }
	static std::string Plugin_GetSourcePath(Script* Self) { return ToStr(Self->ScriptPath.u8string()); }
	static bool Plugin_GetEnabled(Script* Self) { return Self->Enabled; }
	static void Plugin_SetEnabled(bool Enabled, Script* Self) { SetPluginEnabled(Self, Enabled); }
	static bool Plugin_GetFavorite(Script* Self) { return Self->Favorite; }
	static void Plugin_SetFavorite(bool Favorite, Script* Self) { if (TheScriptManager) TheScriptManager->SetFavorite(*Self, Favorite); }
	static bool Plugin_GetUnstable(Script*) { return false; }
	static void Plugin_Enable(Script* Self) { SetPluginEnabled(Self, true); }
	static void Plugin_Disable(Script* Self) { SetPluginEnabled(Self, false); }

	static CScriptArray* Plugin_GetSettings(Script* Self)
	{
		CScriptArray* Array = CScriptArray::Create(s_Engine->GetTypeInfoByDecl("array<Meta::PluginSetting@>"));
		for (const auto& Setting : Self->Settings)
		{
			PluginSetting* Handle = Setting.get();
			Array->InsertLast(&Handle);
		}
		return Array;
	}

	static PluginSetting* Plugin_GetSetting(const std::string& VarName, Script* Self)
	{
		for (const auto& Setting : Self->Settings)
		{
			if (Setting->VarName == VarName) return Setting.get();
		}
		return nullptr;
	}

	static bool Plugin_HasVisibleSettings(Script* Self)
	{
		return std::any_of(Self->Settings.begin(), Self->Settings.end(), [](const std::shared_ptr<PluginSetting>& Setting) { return !Setting->Hidden; });
	}

	static bool CheckSettingType(const PluginSetting* Self, SettingType Type)
	{
		if (Self->Type == Type && Self->Address) return true;
		if (asIScriptContext* Ctx = asGetActiveContext()) Ctx->SetException("Trying to read from setting with wrong type!");
		return false;
	}

	template <typename T>
	static T ReadSetting(const PluginSetting* Self, SettingType Type)
	{
		T Value{};
		if (CheckSettingType(Self, Type)) std::memcpy(&Value, Self->Address, sizeof(T));
		return Value;
	}

	template <typename T>
	static void WriteSetting(PluginSetting* Self, SettingType Type, const T& Value)
	{
		if (CheckSettingType(Self, Type)) std::memcpy(Self->Address, &Value, sizeof(T));
	}

#define SETTING_VALUE(Name_, T_, Type_) \
	static T_ Setting_Read##Name_(PluginSetting* Self) { return ReadSetting<T_>(Self, SettingType::Type_); } \
	static void Setting_Write##Name_(T_ Value, PluginSetting* Self) { WriteSetting(Self, SettingType::Type_, Value); }
#define SETTING_VALUE_REF(Name_, T_, Type_) \
	static T_ Setting_Read##Name_(PluginSetting* Self) { return ReadSetting<T_>(Self, SettingType::Type_); } \
	static void Setting_Write##Name_(const T_& Value, PluginSetting* Self) { WriteSetting(Self, SettingType::Type_, Value); }

	SETTING_VALUE(Bool, bool, Bool)
	SETTING_VALUE(Enum, int32_t, Enum)
	SETTING_VALUE(Float, float, Float)
	SETTING_VALUE(Double, double, Double)
	SETTING_VALUE(Int8, int8_t, Int8)
	SETTING_VALUE(Int16, int16_t, Int16)
	SETTING_VALUE(Int32, int32_t, Int32)
	SETTING_VALUE(Uint8, uint8_t, Uint8)
	SETTING_VALUE(Uint16, uint16_t, Uint16)
	SETTING_VALUE(Uint32, uint32_t, Uint32)
	SETTING_VALUE_REF(Vec2, Math::vec2, Vec2)
	SETTING_VALUE_REF(Vec3, Math::vec3, Vec3)
	SETTING_VALUE_REF(Vec4, Math::vec4, Vec4)

	static int32_t Setting_ReadEnumName(std::string& Name, PluginSetting* Self)
	{
		if (!CheckSettingType(Self, SettingType::Enum)) return 0;

		int32_t Value = *(const int32_t*)Self->Address;
		Name = SettingEnumValueName(Self->EnumType, Value);
		return Value;
	}

	static std::string Setting_ReadString(PluginSetting* Self)
	{
		return CheckSettingType(Self, SettingType::String) ? *(const std::string*)Self->Address : std::string();
	}

	static void Setting_WriteString(const std::string& Value, PluginSetting* Self)
	{
		if (CheckSettingType(Self, SettingType::String)) *(std::string*)Self->Address = Value;
	}

	static std::string Setting_GetNamespace(PluginSetting* Self) { return Self->Namespace; }
	static std::string Setting_GetVarName(PluginSetting* Self) { return Self->VarName; }
	static std::string Setting_GetName(PluginSetting* Self) { return Self->Name; }
	static SettingType Setting_GetType(PluginSetting* Self) { return Self->Type; }
	static std::string Setting_GetTypeName(PluginSetting* Self) { return Self->TypeName; }
	static std::string Setting_GetDescription(PluginSetting* Self) { return Self->Description; }
	static std::string Setting_GetCategory(PluginSetting* Self) { return Self->Category; }
	static bool Setting_GetVisible(PluginSetting* Self) { return !Self->Hidden; }

	static void Setting_Reset(PluginSetting* Self)
	{
		Self->Reset();
	}

	static void Setting_Render(PluginSetting* Self)
	{
		if (TheScriptManager && TheScriptManager->IsExecutingRenderCallback()) RenderSetting(*Self);
	}

	static void SaveSettings()
	{
		if (TheScriptManager) TheScriptManager->SaveSettings();
	}

	static void OpenSettings(Script* Plugin)
	{
		if (TheScriptManager) TheScriptManager->OpenSettings(Plugin ? Plugin : CallingScript());
	}

	static Script* ExecutingPlugin()
	{
		return CallingScript();
	}

	static CScriptArray* AllPlugins()
	{
		CScriptArray* Array = CScriptArray::Create(s_Engine->GetTypeInfoByDecl("array<Meta::Plugin@>"));
		if (!TheScriptManager) return Array;

		for (Script& Plugin : TheScriptManager->Scripts)
		{
			if (!IsLoaded(Plugin)) continue;
			Script* Handle = &Plugin;
			Array->InsertLast(&Handle);
		}
		return Array;
	}

	static CScriptArray* UnloadedPlugins()
	{
		CScriptArray* Array = CScriptArray::Create(s_Engine->GetTypeInfoByDecl("array<Meta::UnloadedPluginInfo>"));
		if (!TheScriptManager) return Array;

		for (Script& Plugin : TheScriptManager->Scripts)
		{
			if (IsLoaded(Plugin)) continue;
			UnloadedPluginInfo Info{ ToStr(Plugin.ScriptPath.u8string()), ToStr(Plugin.Id), PluginSource::UserFolder, TypeOf(Plugin) };
			Array->InsertLast(&Info);
		}
		return Array;
	}

	static Script* GetPluginFromID(const std::string& Id)
	{
		Script* Plugin = TheScriptManager ? TheScriptManager->FindScript(Id) : nullptr;
		return Plugin and IsLoaded(*Plugin) ? Plugin : nullptr;
	}

	static Script* GetPluginFromSiteID(int32_t SiteId)
	{
		if (!TheScriptManager or SiteId == 0) return nullptr;
		for (Script& Plugin : TheScriptManager->Scripts)
		{
			if (IsLoaded(Plugin) and Plugin.Manifest.SiteId == SiteId) return &Plugin;
		}
		return nullptr;
	}

	static bool IsDeveloperMode()
	{
#ifdef _DEBUG
		return true;
#else
		return false;
#endif
	}

	static void UnloadedPluginInfo_Construct(UnloadedPluginInfo* Self) { new (Self) UnloadedPluginInfo(); }
	static void UnloadedPluginInfo_ConstructCopy(const UnloadedPluginInfo& Other, UnloadedPluginInfo* Self) { new (Self) UnloadedPluginInfo(Other); }
	static void UnloadedPluginInfo_Destruct(UnloadedPluginInfo* Self) { Self->~UnloadedPluginInfo(); }
	static UnloadedPluginInfo& UnloadedPluginInfo_Assign(const UnloadedPluginInfo& Other, UnloadedPluginInfo* Self) { return *Self = Other; }

	/*
	!CompatMeta
	for     Meta
	with    Plugin@ ExecutingPlugin()
	with    Plugin@[]@ AllPlugins()
	with    Plugin@ GetPluginFromID(const string&in id)
	with    Plugin@ GetPluginFromSiteID(int siteID)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	META_FN("Plugin@ ExecutingPlugin()", ExecutingPlugin);
	META_FN("Plugin@[]@ AllPlugins()", AllPlugins);
	META_FN("Plugin@ GetPluginFromID(const string&in id)", GetPluginFromID);
	META_FN("Plugin@ GetPluginFromSiteID(int siteID)", GetPluginFromSiteID);

	/*
	!CompatMeta
	for     Meta
	with    bool IsDeveloperMode()
	being   function
	declare partial
	reason  True in debug builds, which are the builds that define DEVELOPER
	since   v0.2.0
	!CompatMeta
	*/
	META_FN("bool IsDeveloperMode()", IsDeveloperMode);

	/*
	!CompatMeta
	for     Meta
	with    string get_ID()
	with    Meta::PluginType get_Type()
	with    string get_Name()
	with    string get_Author()
	with    string get_Category()
	with    string[]@ get_Blocks()
	with    string[]@ get_Dependencies()
	with    string[]@ get_OptionalDependencies()
	with    bool get_HasManifest()
	with    int get_SiteID()
	with    string get_Version()
	with    bool get_Essential()
	with    string get_SourcePath()
	with    bool get_Enabled()
	with    void set_Enabled(bool)
	with    void Enable()
	with    void Disable()
	being   method
	of      Plugin
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("string get_ID() property", Plugin_GetID);
	PLUGIN_METHOD("Meta::PluginType get_Type() property", Plugin_GetType);
	PLUGIN_METHOD("string get_Name() property", Plugin_GetName);
	PLUGIN_METHOD("string get_Author() property", Plugin_GetAuthor);
	PLUGIN_METHOD("string get_Category() property", Plugin_GetCategory);
	PLUGIN_METHOD("string[]@ get_Blocks() property", Plugin_GetBlocks);
	PLUGIN_METHOD("string[]@ get_Dependencies() property", Plugin_GetDependencies);
	PLUGIN_METHOD("string[]@ get_OptionalDependencies() property", Plugin_GetOptionalDependencies);
	PLUGIN_METHOD("bool get_HasManifest() property", Plugin_GetHasManifest);
	PLUGIN_METHOD("int get_SiteID() property", Plugin_GetSiteID);
	PLUGIN_METHOD("string get_Version() property", Plugin_GetVersion);
	PLUGIN_METHOD("bool get_Essential() property", Plugin_GetEssential);
	PLUGIN_METHOD("string get_SourcePath() property", Plugin_GetSourcePath);
	PLUGIN_METHOD("bool get_Enabled() property", Plugin_GetEnabled);
	PLUGIN_METHOD("void set_Enabled(bool) property", Plugin_SetEnabled);
	PLUGIN_METHOD("void Enable()", Plugin_Enable);
	PLUGIN_METHOD("void Disable()", Plugin_Disable);

	/*
	!CompatMeta
	for     Meta
	with    int get_SignatureLevel()
	being   method
	of      Plugin
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("int get_SignatureLevel() property", Plugin_GetSignatureLevel);

	/*
	!CompatMeta
	for     Meta
	with    Meta::PluginSource get_Source()
	being   method
	of      Plugin
	declare partial
	reason  Plugins are only loaded from the user's TwinkieX folder, so this is always UserFolder
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("Meta::PluginSource get_Source() property", Plugin_GetSource);

	/*
	!CompatMeta
	for     Meta
	with    bool get_Favorite()
	with    void set_Favorite(bool)
	with    Meta::PluginSetting@[]@ GetSettings()
	with    Meta::PluginSetting@ GetSetting(const string&in varName)
	with    bool HasVisibleSettings()
	being   method
	of      Plugin
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("bool get_Favorite() property", Plugin_GetFavorite);
	PLUGIN_METHOD("void set_Favorite(bool) property", Plugin_SetFavorite);
	PLUGIN_METHOD("Meta::PluginSetting@[]@ GetSettings()", Plugin_GetSettings);
	PLUGIN_METHOD("Meta::PluginSetting@ GetSetting(const string&in varName)", Plugin_GetSetting);
	PLUGIN_METHOD("bool HasVisibleSettings()", Plugin_HasVisibleSettings);

	/*
	!CompatMeta
	for     Meta
	with    void OpenSettings(Plugin@ plugin = null)
	with    void SaveSettings()
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	META_FN("void OpenSettings(Plugin@ plugin = null)", OpenSettings);
	META_FN("void SaveSettings()", SaveSettings);

	/*
	!CompatMeta
	for     Meta
	with    string get_Namespace()
	with    string get_VarName()
	with    string get_Name()
	with    Meta::PluginSettingType get_Type()
	with    string get_TypeName()
	with    string get_Description()
	with    string get_Category()
	with    bool get_Visible()
	with    void Reset()
	with    void Render()
	with    bool ReadBool()
	with    int ReadEnum()
	with    int ReadEnum(string&out name)
	with    float ReadFloat()
	with    double ReadDouble()
	with    int8 ReadInt8()
	with    int16 ReadInt16()
	with    int ReadInt32()
	with    string ReadString()
	with    vec2 ReadVec2()
	with    vec3 ReadVec3()
	with    vec4 ReadVec4()
	with    uint8 ReadUint8()
	with    uint16 ReadUint16()
	with    uint ReadUint32()
	with    void WriteBool(bool)
	with    void WriteEnum(int)
	with    void WriteFloat(float)
	with    void WriteDouble(double)
	with    void WriteInt8(int8)
	with    void WriteInt16(int16)
	with    void WriteInt32(int)
	with    void WriteString(const string&in)
	with    void WriteVec2(const vec2&in)
	with    void WriteVec3(const vec3&in)
	with    void WriteVec4(const vec4&in)
	with    void WriteUint8(uint8)
	with    void WriteUint16(uint16)
	with    void WriteUint32(uint)
	being   method
	of      PluginSetting
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	SETTING_METHOD("string get_Namespace() property", Setting_GetNamespace);
	SETTING_METHOD("string get_VarName() property", Setting_GetVarName);
	SETTING_METHOD("string get_Name() property", Setting_GetName);
	SETTING_METHOD("Meta::PluginSettingType get_Type() property", Setting_GetType);
	SETTING_METHOD("string get_TypeName() property", Setting_GetTypeName);
	SETTING_METHOD("string get_Description() property", Setting_GetDescription);
	SETTING_METHOD("string get_Category() property", Setting_GetCategory);
	SETTING_METHOD("bool get_Visible() property", Setting_GetVisible);
	SETTING_METHOD("void Reset()", Setting_Reset);
	SETTING_METHOD("void Render()", Setting_Render);
	SETTING_METHOD("bool ReadBool()", Setting_ReadBool);
	SETTING_METHOD("int ReadEnum()", Setting_ReadEnum);
	SETTING_METHOD("int ReadEnum(string&out name)", Setting_ReadEnumName);
	SETTING_METHOD("float ReadFloat()", Setting_ReadFloat);
	SETTING_METHOD("double ReadDouble()", Setting_ReadDouble);
	SETTING_METHOD("int8 ReadInt8()", Setting_ReadInt8);
	SETTING_METHOD("int16 ReadInt16()", Setting_ReadInt16);
	SETTING_METHOD("int ReadInt32()", Setting_ReadInt32);
	SETTING_METHOD("string ReadString()", Setting_ReadString);
	SETTING_METHOD("vec2 ReadVec2()", Setting_ReadVec2);
	SETTING_METHOD("vec3 ReadVec3()", Setting_ReadVec3);
	SETTING_METHOD("vec4 ReadVec4()", Setting_ReadVec4);
	SETTING_METHOD("uint8 ReadUint8()", Setting_ReadUint8);
	SETTING_METHOD("uint16 ReadUint16()", Setting_ReadUint16);
	SETTING_METHOD("uint ReadUint32()", Setting_ReadUint32);
	SETTING_METHOD("void WriteBool(bool)", Setting_WriteBool);
	SETTING_METHOD("void WriteEnum(int)", Setting_WriteEnum);
	SETTING_METHOD("void WriteFloat(float)", Setting_WriteFloat);
	SETTING_METHOD("void WriteDouble(double)", Setting_WriteDouble);
	SETTING_METHOD("void WriteInt8(int8)", Setting_WriteInt8);
	SETTING_METHOD("void WriteInt16(int16)", Setting_WriteInt16);
	SETTING_METHOD("void WriteInt32(int)", Setting_WriteInt32);
	SETTING_METHOD("void WriteString(const string&in)", Setting_WriteString);
	SETTING_METHOD("void WriteVec2(const vec2&in)", Setting_WriteVec2);
	SETTING_METHOD("void WriteVec3(const vec3&in)", Setting_WriteVec3);
	SETTING_METHOD("void WriteVec4(const vec4&in)", Setting_WriteVec4);
	SETTING_METHOD("void WriteUint8(uint8)", Setting_WriteUint8);
	SETTING_METHOD("void WriteUint16(uint16)", Setting_WriteUint16);
	SETTING_METHOD("void WriteUint32(uint)", Setting_WriteUint32);

	/*
	!CompatMeta
	for     Meta
	with    bool get_Unstable()
	being   method
	of      Plugin
	declare partial
	reason  TwinkieX doesn't track unstable plugins, so this is always false
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("bool get_Unstable() property", Plugin_GetUnstable);

	/*
	!CompatMeta
	for     Meta
	with    UnloadedPluginInfo()
	with    UnloadedPluginInfo(const Meta::UnloadedPluginInfo&in)
	with    ~UnloadedPluginInfo()
	being   behavior
	of      UnloadedPluginInfo
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	UNLOADEDPLUGININFO_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f()", UnloadedPluginInfo_Construct);
	UNLOADEDPLUGININFO_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(const Meta::UnloadedPluginInfo&in)", UnloadedPluginInfo_ConstructCopy);
	UNLOADEDPLUGININFO_BEHAVIOUR(asBEHAVE_DESTRUCT, "void f()", UnloadedPluginInfo_Destruct);
	UNLOADEDPLUGININFO_METHOD("Meta::UnloadedPluginInfo& opAssign(const Meta::UnloadedPluginInfo&in)", UnloadedPluginInfo_Assign);

	/*
	!CompatMeta
	for     Meta
	with    string Path
	with    string ID
	with    Meta::PluginSource Source
	with    Meta::PluginType Type
	being   prop
	of      UnloadedPluginInfo
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	UNLOADEDPLUGININFO_PROP("string Path", asOFFSET(UnloadedPluginInfo, Path));
	UNLOADEDPLUGININFO_PROP("string ID", asOFFSET(UnloadedPluginInfo, ID));
	UNLOADEDPLUGININFO_PROP("Meta::PluginSource Source", asOFFSET(UnloadedPluginInfo, Source));
	UNLOADEDPLUGININFO_PROP("Meta::PluginType Type", asOFFSET(UnloadedPluginInfo, Type));

	/*
	!CompatMeta
	for     Meta
	with    UnloadedPluginInfo[]@ UnloadedPlugins()
	being   function
	declare partial
	reason  Lists the plugins that were found but failed to load, since TwinkieX loads every plugin it finds
	since   v0.2.0
	!CompatMeta
	*/
	META_FN("UnloadedPluginInfo[]@ UnloadedPlugins()", UnloadedPlugins);

	void Registrar(asIScriptEngine* Engine)
	{
		s_Engine = Engine;

		Engine->SetDefaultNamespace("Meta");

		Global::RegisterRunContexts(Engine);

		/*
		!CompatMeta
		for     Meta
		with    PluginType
		with    PluginSource
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterEnum("PluginType");
		Engine->RegisterEnumValue("PluginType", "Unknown", (int)PluginType::Unknown);
		Engine->RegisterEnumValue("PluginType", "Legacy", (int)PluginType::Legacy);
		Engine->RegisterEnumValue("PluginType", "Folder", (int)PluginType::Folder);
		Engine->RegisterEnumValue("PluginType", "Zip", (int)PluginType::Zip);

		Engine->RegisterEnum("PluginSource");
		Engine->RegisterEnumValue("PluginSource", "Unknown", (int)PluginSource::Unknown);
		Engine->RegisterEnumValue("PluginSource", "ApplicationFolder", (int)PluginSource::ApplicationFolder);
		Engine->RegisterEnumValue("PluginSource", "UserFolder", (int)PluginSource::UserFolder);

		/*
		!CompatMeta
		for     Meta
		with    PluginSettingType
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterEnum("PluginSettingType");
		Engine->RegisterEnumValue("PluginSettingType", "Unknown", (int)SettingType::Unknown);
		Engine->RegisterEnumValue("PluginSettingType", "Bool", (int)SettingType::Bool);
		Engine->RegisterEnumValue("PluginSettingType", "Enum", (int)SettingType::Enum);
		Engine->RegisterEnumValue("PluginSettingType", "Float", (int)SettingType::Float);
		Engine->RegisterEnumValue("PluginSettingType", "Double", (int)SettingType::Double);
		Engine->RegisterEnumValue("PluginSettingType", "Int8", (int)SettingType::Int8);
		Engine->RegisterEnumValue("PluginSettingType", "Int16", (int)SettingType::Int16);
		Engine->RegisterEnumValue("PluginSettingType", "Int32", (int)SettingType::Int32);
		Engine->RegisterEnumValue("PluginSettingType", "String", (int)SettingType::String);
		Engine->RegisterEnumValue("PluginSettingType", "Vec2", (int)SettingType::Vec2);
		Engine->RegisterEnumValue("PluginSettingType", "Vec3", (int)SettingType::Vec3);
		Engine->RegisterEnumValue("PluginSettingType", "Vec4", (int)SettingType::Vec4);
		Engine->RegisterEnumValue("PluginSettingType", "Uint8", (int)SettingType::Uint8);
		Engine->RegisterEnumValue("PluginSettingType", "Uint16", (int)SettingType::Uint16);
		Engine->RegisterEnumValue("PluginSettingType", "Uint32", (int)SettingType::Uint32);

		Engine->RegisterObjectType("Plugin", 0, asOBJ_REF | asOBJ_NOCOUNT);
		Engine->RegisterObjectType("PluginSetting", 0, asOBJ_REF | asOBJ_NOCOUNT);
		Engine->RegisterObjectType("UnloadedPluginInfo", sizeof(UnloadedPluginInfo), asOBJ_VALUE | asGetTypeTraits<UnloadedPluginInfo>());
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
