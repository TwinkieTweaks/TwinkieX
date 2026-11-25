#include "pch.h"
#include "Meta.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/ScriptFiles.h>

namespace Gecko::Exports::Meta
{
	static std::vector<GeckoExport> s_Exports;
	static asIScriptEngine* s_Engine = nullptr;

#define META_FN(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "Meta", Decl_, Fn_)
#define PLUGIN_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Meta", "Plugin", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_BEHAVIOUR(Beh_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "Meta", "UnloadedPluginInfo", Beh_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Meta", "UnloadedPluginInfo", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define UNLOADEDPLUGININFO_PROP(Decl_, Offset_) GECKO_EXPORT_PROP(s_Exports, "Meta", "UnloadedPluginInfo", Decl_, Offset_)

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
	static void Plugin_SetFavorite(bool Favorite, Script* Self) { Self->Favorite = Favorite; }
	static bool Plugin_GetUnstable(Script*) { return false; }
	static void Plugin_Enable(Script* Self) { SetPluginEnabled(Self, true); }
	static void Plugin_Disable(Script* Self) { SetPluginEnabled(Self, false); }

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
	being   method
	of      Plugin
	declare partial
	reason  Favorites aren't saved between sessions yet
	since   v0.2.0
	!CompatMeta
	*/
	PLUGIN_METHOD("bool get_Favorite() property", Plugin_GetFavorite);
	PLUGIN_METHOD("void set_Favorite(bool) property", Plugin_SetFavorite);

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

		Engine->RegisterObjectType("Plugin", 0, asOBJ_REF | asOBJ_NOCOUNT);
		Engine->RegisterObjectType("UnloadedPluginInfo", sizeof(UnloadedPluginInfo), asOBJ_VALUE | asGetTypeTraits<UnloadedPluginInfo>());
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
