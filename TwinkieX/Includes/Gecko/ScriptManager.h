#pragma once
#include "pch.h"
#include "Script.h"
#include "Awaitable.h"
#include <functional>

#include <ZipArchive/ZipArchive.h>
#include <Twinkie/TwinkLogs/AngelscriptLoggert/AngelscriptLoggert.h>

class CMwNod;

namespace Gecko
{
	void ScriptCtxLineCallback(asIScriptContext* Ctx, std::chrono::time_point<std::chrono::system_clock>* Timeout);

	// Must be synced with CallbackType
	extern const char* CallbackTypeSignatures[24];

	// Must be synced with CallbackTypeSignatures
	enum class CallbackType : uint32_t
	{
		// void Main()
		Main = 0,
		// void Render()
		Render,
		// void RenderInterface()
		RenderInterface,
		// void RenderMenu()
		RenderMenu,
		// void RenderMenuMain()
		RenderMenuMain,
		// void RenderSettings()
		RenderSettings,
		// void RenderEarly()
		RenderEarly,
		// void Update(float dt)
		Update,
		// void SimulationStep()
		SimulationStep,
		// void OnDisabled()
		OnDisabled,
		// void OnEnabled()
		OnEnabled,
		// void OnDestroyed()
		OnDestroyed,
		// void OnSettingsChanged()
		OnSettingsChanged,
		// void OnSettingsSave(Settings::Section& section)
		OnSettingsSave,
		// void OnSettingsLoad(Settings::Section& section)
		OnSettingsLoad,
		// void OnKeyPress(bool down, VirtualKey key)
		OnKeyPress_VoidReturn,
		// UI::InputBlocking OnKeyPress(bool down, VirtualKey key)
		OnKeyPress_InputBlockingReturn,
		// void OnMouseButton(bool down, int button, int x, int y)
		OnMouseButton_VoidReturn,
		// UI::InputBlocking OnMouseButton(bool down, int button, int x, int y)
		OnMouseButton_InputBlockingReturn,
		// void OnMouseMove(int x, int y)
		OnMouseMove,
		// void OnMouseWheel(int x, int y)
		OnMouseWheel_VoidReturn,
		// UI::InputBlocking OnMouseWheel(int x, int y)
		OnMouseWheel_InputBlockingReturn,
		// void OnProtocolUrl(const string &in path)
		OnProtocolUrl,
		// void OnLoadCallback(CMwNod@ nod)
		OnLoadCallback
	};

	class ScriptManager
	{
	public:
		std::vector<Script> Scripts;
		asIScriptEngine* Engine = asCreateScriptEngine();
		CScriptBuilder ScriptBuilder;
		intmax_t ActiveScriptIdx = -1;
		CallbackType ActiveCallbackType = CallbackType::Main;
		std::chrono::milliseconds TimeoutDurationMs = 1000ms;
		std::filesystem::path ImportsPath;
		bool SettingsOpenRequested = false;
		bool Running = false;
		uint32_t MainThreadId = 0;
		bool GameCallbacksInstalled = false;

		ScriptManager();
		~ScriptManager();

		Script* GetActiveScript();
		asIScriptContext* GetActiveContext();
		Script* GetScriptFromCtx(asIScriptContext* Ctx);
		bool IsExecutingRenderCallback();
		WaitState* CurrentWaitState();
		void StopCoroutines(Script&);
		void RunMainTick(Script&);
		void RunCoroutines(Script&, bool MainTick, RunContext Context);
		void RunContextTick(RunContext);
		void InvokeLoadCallbacks(CMwNod* Nod, uint32_t ClassId);

		void RenderDevMenu();
		bool HasSettingsUi(const Script&);
		void RenderScriptSettings(size_t ScriptIdx);
		void SetEnabled(size_t ScriptIdx, bool Enabled);
		void SetFavorite(Script&, bool Favorite);
		void LoadScriptSettings(Script&);
		void UnloadScriptSettings(Script&);
		void SaveScriptSettings(Script&);
		void SaveSettings();
		void RegisterPluginStates();
		void UnregisterPluginStates();
		void OpenSettings(const Script*);
		bool CallFunction(Script&, asIScriptFunction*, CallbackType, const std::function<void(asIScriptContext*)>& SetArgs = {}, bool* Result = nullptr);
		void BuildAll();
		void ResolvePlugins();
		void BindImports();
		Script* FindScript(std::string_view Identifier);
		bool BuildScript(Script&);
		void BuildScriptByCallback(Script&, CallbackType);
		bool RunAll(CallbackType CallbackType, const std::function<void(asIScriptContext*)>& SetArgs = {});
		bool RunCallback(size_t ScriptIdx, CallbackType CallbackType, const std::function<void(asIScriptContext*)>& SetArgs = {});
		// Adds a script from an .op archive, or from a folder whose .as files (subfolders included) are built as one script
		void AddFromFile(const std::filesystem::path&, const std::u8string&);
		void ReloadScript(Script&);
		void RebuildScript(Script&);
		// Reads and preprocesses the script's source codes from its ScriptPath
		void LoadSources(Script&);
		void Preprocess(std::u8string& Src, const std::vector<std::string>& Defines = {});
	};

	extern ScriptManager* TheScriptManager;
}