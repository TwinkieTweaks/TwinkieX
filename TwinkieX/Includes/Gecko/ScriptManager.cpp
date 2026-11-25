#include "pch.h"
#include "ScriptManager.h"

#define CCPP_IMPL
#include <ccpp.h>

#include <fstream>
#include <unordered_set>

#include "Exports/Global/Global.h"
#include "Exports/Math/Math.h"
#include "Exports/Text/ExportText.h"
#include "Exports/String/ExportString.h"
#include "Exports/Time/ExportTime.h"
#include "Exports/IO/ExportIO.h"
#include "Exports/Icons/Icons.h"
#include "Exports/Trackmania/Trackmania.h"
#include "Exports/imgui/ExportImgui.h"
#include "Exports/Nvg/ExportNvg.h"
#include "Exports/Reflection/Reflection.h"
#include "Exports/Net/Net.h"
#include "Exports/Dev/Dev.h"
#include "Exports/Meta/Meta.h"

namespace Gecko
{
	ScriptManager* TheScriptManager = nullptr;

	// Must be synced with CallbackType
	const char* CallbackTypeSignatures[] =
	{
		"void Main()",
		"void Render()",
		"void RenderInterface()",
		"void RenderMenu()",
		"void RenderMenuMain()",
		"void RenderSettings()",
		"void RenderEarly()",
		"void Update(float dt)",
		"void SimulationStep()",
		"void OnDisabled()",
		"void OnEnabled()",
		"void OnDestroyed()",
		"void OnSettingsChanged()",
		"void OnSettingsSave(Settings::Section& section)",
		"void OnSettingsLoad(Settings::Section& section)",
		"void OnKeyPress(bool down, VirtualKey key)",
		"UI::InputBlocking OnKeyPress(bool down, VirtualKey key)",
		"void OnMouseButton(bool down, int button, int x, int y)",
		"UI::InputBlocking OnMouseButton(bool down, int button, int x, int y)",
		"void OnMouseMove(int x, int y)",
		"void OnMouseWheel(int x, int y)",
		"UI::InputBlocking OnMouseWheel(int x, int y)",
		"void OnProtocolUrl(const string &in path)",
		"void OnLoadCallback(CMwNod@ nod)"
	};

	void ScriptCtxLineCallback(asIScriptContext* Ctx, std::chrono::time_point<std::chrono::system_clock>* Timeout)
	{
		if (std::chrono::system_clock::now() >= *Timeout)
		{
			Ctx->Suspend();
			TheScriptManager->GetActiveScript()->Enabled = false;
		}
	}

	static std::u8string FromStr(const std::string& that)
	{
		return (char8_t*)that.c_str();
	}

	void MessageCallback(const asSMessageInfo* msg, void* param)
	{
		const char* type = "ERR ";
		if (msg->type == asMSGTYPE_WARNING)
			type = "WARN";
		else if (msg->type == asMSGTYPE_INFORMATION)
			type = "INFO";
#ifdef _DEBUG
		printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
#endif
		auto message = FromStr(std::format("{} ({}, {}) : {} : {}", msg->section, msg->row, msg->col, type, msg->message));
		if (msg->type == asMSGTYPE_WARNING)
			LoggertAngelscript.warn (message);
		else if (msg->type == asMSGTYPE_INFORMATION)
			LoggertAngelscript.print(message);
		else
			LoggertAngelscript.error(message);
	}

	static void LogException(const Script& Script, asIScriptContext* Ctx)
	{
		const char* Section = nullptr;
		int Line = Ctx->GetExceptionLineNumber(nullptr, &Section);
		asIScriptFunction* Function = Ctx->GetExceptionFunction();
		LoggertAngelscript.error(FromStr(std::format("[{}] Exception '{}' in {} ({}, line {})", (const char*)Script.Id.c_str(), Ctx->GetExceptionString(), Function ? Function->GetDeclaration() : "?", Section ? Section : "?", Line)));
	}

	static std::u8string ToU8(std::string_view Str)
	{
		return std::u8string((const char8_t*)Str.data(), Str.size());
	}

	static bool IsSameIdentifier(std::u8string_view Identifier, std::string_view Other)
	{
		if (Identifier.size() != Other.size()) return false;
		for (size_t Idx = 0; Idx < Identifier.size(); Idx++)
		{
			if (std::tolower((uint8_t)Identifier[Idx]) != std::tolower((uint8_t)Other[Idx])) return false;
		}
		return true;
	}

	static std::u8string NormalizeScriptPath(std::string_view Path)
	{
		std::u8string Normal = ToU8(Path);
		std::replace(Normal.begin(), Normal.end(), u8'\\', u8'/');
		while (Normal.starts_with(u8"./")) Normal.erase(0, 2);
		return Normal;
	}

	static std::string DependencyDefine(std::u8string_view Identifier)
	{
		std::string Define = "DEPENDENCY_";
		for (char8_t Char : Identifier)
		{
			Define += std::isalnum((uint8_t)Char) ? (char)std::toupper((uint8_t)Char) : '_';
		}
		return Define;
	}

	static std::chrono::milliseconds ScriptTimeout(const Script& Script, std::chrono::milliseconds Default)
	{
		return Script.Manifest.TimeoutMs ? std::chrono::milliseconds(*Script.Manifest.TimeoutMs) : Default;
	}

	static int ExecuteContext(Script& Script, asIScriptContext* Ctx, [[maybe_unused]] std::chrono::milliseconds Timeout)
	{
#ifndef _DEBUG
		static std::chrono::time_point<std::chrono::system_clock> When;
		When = std::chrono::system_clock::now() + Timeout;

		if (Timeout.count() > 0) Ctx->SetLineCallback(asFUNCTION(ScriptCtxLineCallback), &When, asCALL_CDECL);
		else Ctx->ClearLineCallback();
#endif
		int r = Ctx->Execute();
		if (r == asEXECUTION_EXCEPTION) LogException(Script, Ctx);
		return r;
	}

	ScriptManager::ScriptManager()
	{
		Engine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 2);
		Engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, true);
		Engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

		// The math types come first, since the game's classes have members of those types
		Exports::Math::RegisterTypes(Engine);
		Exports::Global::RegisterBaseTypes(Engine);
		Exports::String::RegisterWString(Engine);
		Exports::Trackmania::Registrar(Engine);
		Exports::Global::Registrar(Engine);
		Exports::String::Registrar(Engine);
		Exports::Time::Registrar(Engine);
		Exports::IO::Registrar(Engine);
		Exports::Math::Registrar(Engine);
		Exports::Text::Registrar(Engine);
		Exports::Icons::Registrar(Engine);
		Exports::ImGui::Registrar(Engine);
		Exports::Nvg::Registrar(Engine);
		Exports::Net::Registrar(Engine);
		Exports::Reflection::Registrar(Engine);
		Exports::Dev::Registrar(Engine);
		Exports::Meta::Registrar(Engine);
	}

	ScriptManager::~ScriptManager()
	{
		for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
		{
			RunCallback(Idx, CallbackType::OnDestroyed);
			StopCoroutines(Scripts[Idx]);
			Scripts[Idx].Ctxs.ReleaseAll();
		}

		Engine->ShutDownAndRelease();

		Exports::Trackmania::Cleanup();
	}

	Script* ScriptManager::GetActiveScript()
	{
		return &(Scripts[ActiveScriptIdx]);
	}

	asIScriptContext* ScriptManager::GetActiveContext()
	{
		return Scripts[ActiveScriptIdx].Ctxs[(size_t)ActiveCallbackType];
	}

	void ScriptManager::BuildScriptByCallback(Script& Script, CallbackType CallbackType)
	{
		asIScriptContext*& Ctx = Script.Ctxs[(size_t)CallbackType];
		if (Ctx)
		{
			Ctx->Release();
			Ctx = nullptr;
		}

		auto Module = Engine->GetModule((const char*)Script.Name.c_str());
		auto CallbackFn = Module ? Module->GetFunctionByDecl(CallbackTypeSignatures[(size_t)CallbackType]) : nullptr;
		if (!CallbackFn) return;

		Ctx = Engine->CreateContext();
		if (Ctx->Prepare(CallbackFn) < 0)
		{
			Ctx->Release();
			Ctx = nullptr;
		}
	}

	bool ScriptManager::IsExecutingRenderCallback()
	{
		switch (ActiveCallbackType)
		{
		case CallbackType::Render:
		case CallbackType::RenderInterface:
		case CallbackType::RenderMenu:
		case CallbackType::RenderMenuMain:
		case CallbackType::RenderSettings:
		case CallbackType::RenderEarly:
			return true;
		default:
			return false;
		}
	}

	WaitState* ScriptManager::CurrentWaitState()
	{
		asIScriptContext* Ctx = asGetActiveContext();
		if (!Ctx || Ctx->IsNested() || ActiveCallbackType != CallbackType::Main) return nullptr;
		if (ActiveScriptIdx < 0 || ActiveScriptIdx >= (intmax_t)Scripts.size()) return nullptr;

		Script& Current = Scripts[ActiveScriptIdx];
		if (Ctx == Current.Ctxs[(size_t)CallbackType::Main]) return &Current.MainWait;
		for (Coroutine* Co : Current.Coroutines)
		{
			if (Co->Ctx == Ctx) return &Co->Wait;
		}
		return nullptr;
	}

	void ScriptManager::StopCoroutines(Script& Script)
	{
		for (Coroutine* Co : Script.Coroutines)
		{
			Co->Stop();
			Co->Release();
		}
		Script.Coroutines.clear();
		Script.MainWait.Clear();
	}

	void ScriptManager::RunMainTick(Script& Script)
	{
		asIScriptContext*& Main = Script.Ctxs[(size_t)CallbackType::Main];
		if (Main && Script.MainWait.Ready())
		{
			if (ExecuteContext(Script, Main, ScriptTimeout(Script, TimeoutDurationMs)) != asEXECUTION_SUSPENDED)
			{
				Main->Release();
				Main = nullptr;
				Script.MainWait.Clear();
			}
		}

		for (size_t Idx = 0; Idx < Script.Coroutines.size() && Script.Enabled; Idx++)
		{
			Coroutine* Co = Script.Coroutines[Idx];
			if (!Co->Ctx || !Co->Wait.Ready()) continue;
			if (ExecuteContext(Script, Co->Ctx, ScriptTimeout(Script, TimeoutDurationMs)) != asEXECUTION_SUSPENDED) Co->Stop();
		}

		std::erase_if(Script.Coroutines, [](Coroutine* Co)
		{
			if (Co->Running) return false;
			Co->Release();
			return true;
		});
	}

	Script* ScriptManager::FindScript(std::string_view Identifier)
	{
		for (auto& Script : Scripts)
		{
			if (IsSameIdentifier(Script.Id, Identifier)) return &Script;
		}
		return nullptr;
	}

	void ScriptManager::BuildScript(Script& Script)
	{
		if (!Script.LoadError.empty()) return;

		LoggertAngelscript.trace(FromStr(std::format("Compiling {}", (const char*)Script.Id.c_str())));

		if (&Script >= Scripts.data() && &Script < Scripts.data() + Scripts.size())
		{
			RunCallback((size_t)(&Script - Scripts.data()), CallbackType::OnDestroyed);
		}

		StopCoroutines(Script);

		if (Engine->GetModule((const char*)Script.Name.c_str()))
		{
			Engine->DiscardModule((const char*)Script.Name.c_str());
		}

		std::vector<std::string> Defines = Script.Manifest.Defines;
		std::vector<::Gecko::Script*> ExportSources;
		std::unordered_set<::Gecko::Script*> Visited;

		std::function<void(::Gecko::Script*)> AddExports = [&](::Gecko::Script* Dependency)
		{
			if (!Dependency or !Dependency->LoadError.empty() or Visited.contains(Dependency)) return;
			Visited.insert(Dependency);
			ExportSources.push_back(Dependency);
			for (const std::string& Exported : Dependency->Manifest.ExportDependencies) AddExports(FindScript(Exported));
		};

		for (const std::vector<std::string>* List : { &Script.Manifest.Dependencies, &Script.Manifest.OptionalDependencies })
		{
			for (const std::string& Identifier : *List)
			{
				::Gecko::Script* Dependency = FindScript(Identifier);
				if (!Dependency or !Dependency->LoadError.empty()) continue;

				Defines.push_back(DependencyDefine(Dependency->Id));
				AddExports(Dependency);
			}
		}

		ScriptBuilder.StartNewModule(Engine, (const char*)Script.Name.c_str());
		for (size_t SourceIdx = 0; SourceIdx < Script.SourceCodes.size(); SourceIdx++)
		{
			// The builder skips sections whose name was already added, so every source needs its own name
			const std::u8string& SectionName = SourceIdx < Script.SourceNames.size() ? Script.SourceNames[SourceIdx] : Script.Name;
			LoggertAngelscript.trace(FromStr(std::format("Compiling {}", (const char*)SectionName.c_str())));

			std::u8string SourceCode = Script.SourceCodes[SourceIdx];
			Preprocess(SourceCode, Defines);
			ScriptBuilder.AddSectionFromMemory((const char*)SectionName.c_str(), (const char*)SourceCode.c_str());
		}
		for (::Gecko::Script* Dependency : ExportSources)
		{
			for (size_t ExportIdx = 0; ExportIdx < Dependency->ExportCodes.size(); ExportIdx++)
			{
				std::u8string SectionName = Dependency->Id + u8"/" + Dependency->ExportNames[ExportIdx];
				LoggertAngelscript.trace(FromStr(std::format("Compiling {}", (const char*)SectionName.c_str())));

				std::u8string SourceCode = Dependency->ExportCodes[ExportIdx];
				Preprocess(SourceCode, Defines);
				ScriptBuilder.AddSectionFromMemory((const char*)SectionName.c_str(), (const char*)SourceCode.c_str());
			}
		}
		ScriptBuilder.BuildModule();

		CallbackType CallbackType = CallbackType::Main;
		for ([[maybe_unused]] auto& _ : CallbackTypeSignatures)
		{
			BuildScriptByCallback(Script, CallbackType);
			(*(uint32_t*)&CallbackType)++;
		}
	}

	void ScriptManager::ResolvePlugins()
	{
		auto Fail = [](Script& Script, std::string Error)
		{
			if (!Script.LoadError.empty()) return;
			LoggertAngelscript.error(FromStr(std::format("[{}] Not loaded: {}", (const char*)Script.Id.c_str(), Error)));
			Script.LoadError = std::move(Error);
			Script.Enabled = false;
		};

		for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
		{
			for (size_t Other = 0; Other < Idx; Other++)
			{
				if (Scripts[Other].Id == Scripts[Idx].Id) Fail(Scripts[Idx], std::format("another plugin already uses the identifier {}", (const char*)Scripts[Idx].Id.c_str()));
				else if (Scripts[Other].Name == Scripts[Idx].Name) Fail(Scripts[Idx], std::format("another plugin already uses the module name {}", (const char*)Scripts[Idx].Name.c_str()));
			}
		}

		for (auto& Script : Scripts)
		{
			const std::vector<std::string>& Games = Script.Manifest.SupportedGames;
			if (Games.empty()) continue;

			bool Supported = std::any_of(Games.begin(), Games.end(), [](const std::string& Game)
			{
				return IsSameIdentifier(u8"" TWINKIEX_GAME, Game)
#ifdef TMCN
					or IsSameIdentifier(u8"TMNEXT", Game)
#endif
					;
			});
			if (!Supported) Fail(Script, std::format("{} isn't in its supported games", TWINKIEX_GAME));
		}

		for (auto& Script : Scripts)
		{
			if (!Script.LoadError.empty()) continue;
			for (const std::string& Blocked : Script.Manifest.Blocks)
			{
				::Gecko::Script* Target = FindScript(Blocked);
				if (Target and Target != &Script) Fail(*Target, std::format("blocked by {}", (const char*)Script.Id.c_str()));
			}
		}

		auto FailMissingDependencies = [&]
		{
			for (bool Changed = true; Changed;)
			{
				Changed = false;
				for (auto& Script : Scripts)
				{
					if (!Script.LoadError.empty()) continue;
					for (const std::string& Identifier : Script.Manifest.Dependencies)
					{
						::Gecko::Script* Dependency = FindScript(Identifier);
						if (Dependency and Dependency->LoadError.empty()) continue;

						Fail(Script, Dependency ? std::format("its dependency {} isn't loaded", Identifier) : std::format("its dependency {} is missing", Identifier));
						Changed = true;
						break;
					}
				}
			}
		};
		FailMissingDependencies();

		std::vector<size_t> Order;
		std::vector<uint8_t> State(Scripts.size(), 0);
		std::function<bool(size_t)> Visit = [&](size_t Idx) -> bool
		{
			if (State[Idx] == 2) return true;
			if (State[Idx] == 1) return false;
			State[Idx] = 1;

			bool Acyclic = true;
			for (const std::vector<std::string>* List : { &Scripts[Idx].Manifest.Dependencies, &Scripts[Idx].Manifest.OptionalDependencies, &Scripts[Idx].Manifest.ExportDependencies })
			{
				for (const std::string& Identifier : *List)
				{
					::Gecko::Script* Dependency = FindScript(Identifier);
					if (!Dependency or Dependency == &Scripts[Idx]) continue;
					if (!Visit((size_t)(Dependency - Scripts.data()))) Acyclic = false;
				}
			}

			State[Idx] = 2;
			Order.push_back(Idx);
			if (!Acyclic) Fail(Scripts[Idx], "its dependencies form a cycle");
			return Acyclic;
		};
		for (size_t Idx = 0; Idx < Scripts.size(); Idx++) Visit(Idx);
		FailMissingDependencies();

		std::vector<Script> Sorted;
		Sorted.reserve(Scripts.size());
		for (size_t Idx : Order) Sorted.push_back(std::move(Scripts[Idx]));
		Scripts = std::move(Sorted);
	}

	void ScriptManager::BindImports()
	{
		for (auto& Script : Scripts)
		{
			if (!Script.LoadError.empty()) continue;

			asIScriptModule* Module = Engine->GetModule((const char*)Script.Name.c_str());
			if (!Module) continue;

			for (asUINT Idx = 0; Idx < Module->GetImportedFunctionCount(); Idx++)
			{
				const char* Source = Module->GetImportedFunctionSourceModule(Idx);
				asIScriptModule* SourceModule = Source ? Engine->GetModule(Source) : nullptr;
				asIScriptFunction* Function = SourceModule ? SourceModule->GetFunctionByDecl(Module->GetImportedFunctionDeclaration(Idx)) : nullptr;

				if (!Function or Module->BindImportedFunction(Idx, Function) < 0)
				{
					LoggertAngelscript.error(FromStr(std::format("[{}] Couldn't import {} from {}", (const char*)Script.Id.c_str(), Module->GetImportedFunctionDeclaration(Idx), Source ? Source : "?")));
				}
			}
		}
	}

	void ScriptManager::BuildAll()
	{
		ResolvePlugins();
		for (auto& Script : Scripts)
		{
			BuildScript(Script);
		}
		BindImports();
	}

	static bool ReturnsInputBlocking(CallbackType CallbackType)
	{
		return CallbackType == CallbackType::OnKeyPress_InputBlockingReturn
			|| CallbackType == CallbackType::OnMouseButton_InputBlockingReturn
			|| CallbackType == CallbackType::OnMouseWheel_InputBlockingReturn;
	}

	bool ScriptManager::RunAll(CallbackType CallbackType, const std::function<void(asIScriptContext*)>& SetArgs)
	{
		bool Blocked = false;
		for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
		{
			if (RunCallback(Idx, CallbackType, SetArgs)) Blocked = true;
		}
		return Blocked;
	}

	bool ScriptManager::RunCallback(size_t ScriptIdx, CallbackType CallbackType, const std::function<void(asIScriptContext*)>& SetArgs)
	{
		if (Running || ScriptIdx >= Scripts.size()) return false;

		Script& Script = Scripts[ScriptIdx];
		if (!Script.Enabled) return false;
		if (CallbackType != CallbackType::Main && !Script.Ctxs[(size_t)CallbackType]) return false;

		Running = true;
		ActiveScriptIdx = (intmax_t)ScriptIdx;
		ActiveCallbackType = CallbackType;

		bool Blocked = false;
		if (CallbackType == CallbackType::Main)
		{
			RunMainTick(Script);
		}
		else
		{
			asIScriptContext*& Ctx = Script.Ctxs[(size_t)CallbackType];
			if (SetArgs && Ctx->GetState() == asEXECUTION_PREPARED) SetArgs(Ctx);

			int r = ExecuteContext(Script, Ctx, ScriptTimeout(Script, TimeoutDurationMs));
			if (r != asEXECUTION_SUSPENDED)
			{
				if (r == asEXECUTION_FINISHED && ReturnsInputBlocking(CallbackType)) Blocked = Ctx->GetReturnDWord() == 1;

				auto Module = Engine->GetModule((const char*)Script.Name.c_str());
				auto CallbackFn = Module ? Module->GetFunctionByDecl(CallbackTypeSignatures[(size_t)CallbackType]) : nullptr;
				if (!CallbackFn || Ctx->Prepare(CallbackFn) < 0)
				{
					Ctx->Release();
					Ctx = nullptr;
				}
			}
		}

		ActiveCallbackType = CallbackType::Main;
		ActiveScriptIdx = -1;
		Running = false;
		return Blocked;
	}

	void ScriptManager::SetEnabled(size_t ScriptIdx, bool Enabled)
	{
		if (ScriptIdx >= Scripts.size() || Scripts[ScriptIdx].Enabled == Enabled) return;
		if (Enabled && !Scripts[ScriptIdx].LoadError.empty()) return;
		if (!Enabled && Scripts[ScriptIdx].Manifest.Essential) return;

		if (Enabled)
		{
			Scripts[ScriptIdx].Enabled = true;
			RunCallback(ScriptIdx, CallbackType::OnEnabled);
		}
		else
		{
			RunCallback(ScriptIdx, CallbackType::OnDisabled);
			Scripts[ScriptIdx].Enabled = false;
		}
	}

	void ScriptManager::RenderSettings()
	{
		for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
		{
			Script& Current = Scripts[Idx];
			if (!Current.Enabled || !Current.Ctxs[(size_t)CallbackType::RenderSettings]) continue;

			ImGui::PushID((int)Idx);
			if (ImGui::CollapsingHeader((const char*)Current.DisplayName.c_str())) RunCallback(Idx, CallbackType::RenderSettings);
			ImGui::PopID();
		}
	}

	Script* ScriptManager::GetScriptFromCtx(asIScriptContext* Ctx)
	{
		for (auto& Script : Scripts)
		{
			size_t Idx = 0;
			for ([[maybe_unused]] auto& _ : CallbackTypeSignatures)
			{
				if (Script.Ctxs[Idx] == Ctx)
				{
					return &Script;
				}
				Idx++;
			}
		}
		return nullptr;
	}

	void ScriptManager::Preprocess(std::u8string& Src, const std::vector<std::string>& Defines)
	{
		ccpp::processor Preprocessor;

		for (const std::string& Define : Defines) Preprocessor.add_define(Define.c_str());

		{
			Preprocessor.add_define("TWINKIEX");
			Preprocessor.add_define(TWINKIEX_GAME);
#ifdef TMCN
			Preprocessor.add_define("TMNEXT");
#endif
#ifdef _DEBUG
			Preprocessor.add_define("DEVELOPER");
#endif
#ifdef X86
			Preprocessor.add_define("MANIA32");
#else
			Preprocessor.add_define("MANIA64");
#endif
			// TODO: Maybe add a signature toggle?
			Preprocessor.add_define("SIG_DEVELOPER");

			auto NtDll = LoadLibrary(L"ntdll.dll");
			if (NtDll && GetProcAddress(NtDll, "wine_get_version"))
				Preprocessor.add_define("WINDOWS_WINE");

			Preprocessor.add_define("WINDOWS");
		}

		// ...ugh
		char* Buffer = new char[Src.size() + 1];
		strcpy_s(Buffer, Src.size() + 1, (const char*)Src.c_str());

		Preprocessor.process(Buffer, Src.size());
		Src = (char8_t*)Buffer;

		delete[] Buffer;
	}

	static std::u8string ReadWholeFile(const std::filesystem::path& Path)
	{
		std::ifstream Stream(Path, std::ios::binary);
		std::string Contents((std::istreambuf_iterator<char>(Stream)), std::istreambuf_iterator<char>());
		return std::u8string((const char8_t*)Contents.data(), Contents.size());
	}

	void ScriptManager::LoadSources(Script& Script)
	{
		Script.SourceCodes.clear();
		Script.SourceNames.clear();
		Script.ExportCodes.clear();
		Script.ExportNames.clear();
		Script.Manifest = {};
		Script.LoadError.clear();

		std::vector<std::pair<std::u8string, std::u8string>> Files;
		std::optional<std::string> ManifestSource;

		if (std::filesystem::is_directory(Script.ScriptPath))
		{
			try
			{
				for (auto& Entry : std::filesystem::recursive_directory_iterator(Script.ScriptPath, std::filesystem::directory_options::skip_permission_denied))
				{
					if (!Entry.is_regular_file()) continue;

					std::u8string Relative = std::filesystem::relative(Entry.path(), Script.ScriptPath).generic_u8string();
					if (Relative == u8"info.toml")
					{
						std::u8string Contents = ReadWholeFile(Entry.path());
						ManifestSource = std::string((const char*)Contents.data(), Contents.size());
					}
					else if (Entry.path().extension() == ".as")
					{
						Files.emplace_back(Relative, ReadWholeFile(Entry.path()));
					}
				}
			}
			catch (const std::filesystem::filesystem_error& Error)
			{
				LoggertAngelscript.error(FromStr(std::format("Couldn't read the folder of {}: {}", (const char*)Script.Id.c_str(), Error.what())));
			}
		}
		else
		{
			ZipArchive Archive(Script.ScriptPath);
			for (auto& File : Archive)
			{
				std::u8string Name = File.Filename;
				std::replace(Name.begin(), Name.end(), u8'\\', u8'/');

				if (Name == u8"info.toml") ManifestSource = std::string((const char*)File.Data.data(), File.Data.size());
				else if (std::filesystem::path(Name).extension() == ".as") Files.emplace_back(Name, std::u8string(File.Data.begin(), File.Data.end()));
			}
		}

		// Sorted so the build doesn't depend on the order the file system or archive lists files in
		std::sort(Files.begin(), Files.end());

		if (ManifestSource)
		{
			std::string Error;
			if (!ParsePluginManifest(*ManifestSource, Script.Manifest, Error))
			{
				Script.LoadError = "invalid info.toml: " + Error;
				LoggertAngelscript.error(FromStr(std::format("[{}] Not loaded: {}", (const char*)Script.Id.c_str(), Script.LoadError)));
			}
		}

		auto IsListed = [](const std::vector<std::string>& List, const std::u8string& Name)
		{
			return std::any_of(List.begin(), List.end(), [&Name](const std::string& Entry) { return NormalizeScriptPath(Entry) == Name; });
		};

		for (auto& [Name, Code] : Files)
		{
			bool Exported = IsListed(Script.Manifest.Exports, Name);
			if (Exported or IsListed(Script.Manifest.SharedExports, Name))
			{
				Script.ExportNames.push_back(Name);
				Script.ExportCodes.push_back(Code);
			}
			if (Exported) continue;

			Script.SourceNames.push_back(Name);
			Script.SourceCodes.push_back(Code);
		}

		for (const std::string& Import : Script.Manifest.Imports)
		{
			std::u8string Normal = NormalizeScriptPath(Import);
			std::filesystem::path Path = ImportsPath / std::filesystem::path(Normal);
			if (!std::filesystem::is_regular_file(Path))
			{
				LoggertAngelscript.error(FromStr(std::format("[{}] Import {} wasn't found in {}", (const char*)Script.Id.c_str(), Import, (const char*)ImportsPath.u8string().c_str())));
				continue;
			}

			Script.SourceNames.push_back(u8"Imports/" + Normal);
			Script.SourceCodes.push_back(ReadWholeFile(Path));
		}

		if (!Script.Manifest.Module.empty()) Script.Name = ToU8(Script.Manifest.Module);
		else if (Script.Name.empty()) Script.Name = Script.Id;

		Script.DisplayName = Script.Manifest.Name.empty() ? Script.Id : ToU8(Script.Manifest.Name);
		if (!Script.LoadError.empty()) Script.Enabled = false;
	}

	void ScriptManager::AddFromFile(const std::filesystem::path& FilePath, const std::u8string& ScriptName)
	{
		Script NewScript;
		NewScript.ScriptPath = FilePath;
		NewScript.Name = ScriptName;
		NewScript.Id = std::filesystem::is_directory(FilePath) ? FilePath.filename().u8string() : FilePath.stem().u8string();

		LoadSources(NewScript);

		Scripts.push_back(NewScript);
	}

	void ScriptManager::ReloadScript(Script& Script)
	{
		LoadSources(Script);
	}

	void ScriptManager::RenderDevMenu()
	{
		using namespace ImGui;

		if (BeginMenu("Developer"))
		{
			if (BeginMenu("Reload module"))
			{
				for (auto& Module : Scripts)
				{
					if (MenuItem((const char*)Module.DisplayName.c_str()))
					{
						ReloadScript(Module);
						BuildScript(Module);
						BindImports();
					}
				}

				ImGui::EndMenu();
			}
			if (BeginMenu("Enabled modules"))
			{
				for (size_t Idx = 0; Idx < Scripts.size(); Idx++)
				{
					if (MenuItem((const char*)Scripts[Idx].DisplayName.c_str(), nullptr, Scripts[Idx].Enabled, Scripts[Idx].LoadError.empty() && !Scripts[Idx].Manifest.Essential))
					{
						SetEnabled(Idx, !Scripts[Idx].Enabled);
					}
				}

				ImGui::EndMenu();
			}
			// TODO: Rest

			ImGui::EndMenu();
		}
	}
}