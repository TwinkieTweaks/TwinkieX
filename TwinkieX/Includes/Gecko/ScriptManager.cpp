#include "pch.h"
#include "ScriptManager.h"

#define CCPP_IMPL
#include <ccpp.h>

#include <fstream>

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
	}

	ScriptManager::~ScriptManager()
	{
		for (auto& Script : Scripts)
		{
			Script.Ctxs.ReleaseAll();
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

	void ScriptManager::BuildScript(Script& Script)
	{
		LoggertAngelscript.trace(FromStr(std::format("Compiling {}", (const char*)Script.Name.c_str())));

		if (Engine->GetModule((const char*)Script.Name.c_str()))
		{
			Engine->DiscardModule((const char*)Script.Name.c_str());
		}

		ScriptBuilder.StartNewModule(Engine, (const char*)Script.Name.c_str());
		for (size_t SourceIdx = 0; SourceIdx < Script.SourceCodes.size(); SourceIdx++)
		{
			// The builder skips sections whose name was already added, so every source needs its own name
			const std::u8string& SectionName = SourceIdx < Script.SourceNames.size() ? Script.SourceNames[SourceIdx] : Script.Name;
			LoggertAngelscript.trace(FromStr(std::format("Compiling {}", (const char*)SectionName.c_str())));
			ScriptBuilder.AddSectionFromMemory((const char*)SectionName.c_str(), (const char*)Script.SourceCodes[SourceIdx].c_str());
		}
		ScriptBuilder.BuildModule();

		CallbackType CallbackType = CallbackType::Main;
		for ([[maybe_unused]] auto& _ : CallbackTypeSignatures)
		{
			BuildScriptByCallback(Script, CallbackType);
			(*(uint32_t*)&CallbackType)++;
		}
	}

	void ScriptManager::BuildAll()
	{
		for (auto& Script : Scripts)
		{
			BuildScript(Script);
		}
	}

	void ScriptManager::RunAll(CallbackType CallbackType)
	{
		static bool Running = false;
		if (Running) return;
		Running = true;

		ActiveScriptIdx = 0;
		ActiveCallbackType = CallbackType;
		for (auto& Script : Scripts)
		{
			if (!Script.Enabled)
			{
				ActiveScriptIdx++;
				continue;
			}

			asIScriptContext*& CtxFromCallback = Script.Ctxs[(size_t)CallbackType];
			if (!CtxFromCallback)
			{
				ActiveScriptIdx++;
				continue;
			}

			if (CallbackType == CallbackType::Main && Script.YieldFor > 0)
			{
				Script.YieldFor--;
				ActiveScriptIdx++;
				continue;
			}

#ifndef _DEBUG
			static std::chrono::time_point<std::chrono::system_clock> When;
			When = std::chrono::system_clock::now() + TimeoutDurationMs;

			CtxFromCallback->SetLineCallback(asFUNCTION(ScriptCtxLineCallback), &When, asCALL_CDECL);
#endif

			int r = CtxFromCallback->Execute();
			if (r != asEXECUTION_FINISHED)
			{
				if (r == asEXECUTION_EXCEPTION)
				{
					printf("An exception '%s' occurred. Please correct the code and try again.\n", CtxFromCallback->GetExceptionString());
				}
			}

			if (r == asEXECUTION_SUSPENDED)
			{
				goto Continue;
			}

			// Main is only run once. A rebuild is required to re-run it.
			if (CallbackType == CallbackType::Main)
			{
				CtxFromCallback->Release();
				CtxFromCallback = nullptr;
			}
			else
			{
				auto Module = Engine->GetModule((const char*)Script.Name.c_str());
				auto CallbackFn = Module ? Module->GetFunctionByDecl(CallbackTypeSignatures[(size_t)CallbackType]) : nullptr;
				if (!CallbackFn || CtxFromCallback->Prepare(CallbackFn) < 0)
				{
					CtxFromCallback->Release();
					CtxFromCallback = nullptr;
				}
			}
		Continue:
			ActiveScriptIdx++;
		}
		ActiveCallbackType = CallbackType::Main;
		ActiveScriptIdx = -1;
		Running = false;
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

	void ScriptManager::Preprocess(std::u8string& Src)
	{
		ccpp::processor Preprocessor;

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

		if (std::filesystem::is_directory(Script.ScriptPath))
		{
			std::vector<std::filesystem::path> Files;
			try
			{
				for (auto& Entry : std::filesystem::recursive_directory_iterator(Script.ScriptPath, std::filesystem::directory_options::skip_permission_denied))
				{
					if (Entry.is_regular_file() and Entry.path().extension() == ".as") Files.push_back(Entry.path());
				}
			}
			catch (const std::filesystem::filesystem_error& Error)
			{
				LoggertAngelscript.error(FromStr(std::format("Couldn't read the folder of {}: {}", (const char*)Script.Name.c_str(), Error.what())));
			}

			// Sorted so the build doesn't depend on the order the file system lists files in
			std::sort(Files.begin(), Files.end());

			for (auto& File : Files)
			{
				std::u8string SourceCode = ReadWholeFile(File);
				Preprocess(SourceCode);

				Script.SourceCodes.push_back(SourceCode);
				Script.SourceNames.push_back(std::filesystem::relative(File, Script.ScriptPath).generic_u8string());
			}
			return;
		}

		ZipArchive Archive(Script.ScriptPath);

		for (auto& File : Archive)
		{
			// Archives also contain files like info.toml
			if (std::filesystem::path(File.Filename).extension() != ".as") continue;

			std::u8string SourceCode(File.Data.begin(), File.Data.end());
			Preprocess(SourceCode);

			Script.SourceCodes.push_back(SourceCode);
			Script.SourceNames.push_back(File.Filename);
		}
	}

	void ScriptManager::AddFromFile(const std::filesystem::path& FilePath, const std::u8string& ScriptName)
	{
		Script NewScript;
		NewScript.ScriptPath = FilePath;
		NewScript.Name = ScriptName.empty() ? FilePath.u8string() : ScriptName;
		NewScript.Id = FilePath.filename().u8string();

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
					if (MenuItem((const char*)Module.Id.c_str()))
					{
						ReloadScript(Module);
						BuildScript(Module);
					}
				}

				ImGui::EndMenu();
			}
			// TODO: Rest

			ImGui::EndMenu();
		}
	}
}