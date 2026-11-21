#include "pch.h"
#include "Font.h"
#include <Gecko/ScriptManager.h>
#include <Twinkie/TwinkUi/TwinkUi.h>
#include <ZipArchive/ZipArchive.h>
#include <fstream>
#include <map>

struct AtlasFont
{
	ImFont* Font = nullptr;
	std::vector<ImWchar> GlyphRanges;
};
static std::map<std::string, AtlasFont> AtlasFonts;

ScriptFont::ScriptFont(std::string pKey, std::vector<uint8_t> pData, float pSize, std::vector<ImWchar> pGlyphRanges)
	: Key(std::move(pKey)), Data(std::move(pData)), GlyphRanges(std::move(pGlyphRanges)), Size(pSize)
{
}

void ScriptFont::AddRef()
{
	RefCount++;
}

void ScriptFont::RemoveRef()
{
	if (!--RefCount) delete this;
}

static void Warn(const std::string& Message)
{
	LoggertAngelscript.warn(std::u8string(Message.begin(), Message.end()));
}

// TrueType, OpenType, or a TrueType collection
static bool IsFontFile(const std::vector<uint8_t>& Data)
{
	if (Data.size() < 12) return false;
	uint32_t Tag = (uint32_t)Data[0] << 24 | (uint32_t)Data[1] << 16 | (uint32_t)Data[2] << 8 | Data[3];
	return Tag == 0x00010000 or Tag == 0x4F54544F /* OTTO */ or Tag == 0x74727565 /* true */ or Tag == 0x74746366 /* ttcf */;
}

static bool ReadFile(const std::filesystem::path& Path, std::vector<uint8_t>& Out)
{
	std::error_code Error;
	if (not std::filesystem::is_regular_file(Path, Error)) return false;

	std::ifstream File(Path, std::ios::binary);
	if (not File) return false;

	Out.assign(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
	return true;
}

static Gecko::Script* CallingScript()
{
	asIScriptContext* Ctx = asGetActiveContext();
	asIScriptFunction* Function = Ctx ? Ctx->GetFunction(0) : nullptr;
	const char* ModuleName = Function ? Function->GetModuleName() : nullptr;
	if (not ModuleName or not Gecko::TheScriptManager) return nullptr;

	for (auto& Script : Gecko::TheScriptManager->Scripts)
	{
		if (Script.Name == (const char8_t*)ModuleName) return &Script;
	}
	return nullptr;
}

static bool ReadFromFontsFolder(const std::filesystem::path& Relative, std::vector<uint8_t>& Data, std::string& Key)
{
	if (not TwinkUiState::UiMgr) return false;

	std::filesystem::path Path = TwinkUiState::UiMgr->DocumentsFolderPath / L"TwinkieX\\Fonts" / Relative;
	if (not ReadFile(Path, Data)) return false;

	Key = (const char*)Path.u8string().c_str();
	return true;
}

ScriptFont* ScriptFont::Load(const std::string& Filename, float Size, std::vector<ImWchar> GlyphRanges)
{
	std::filesystem::path Relative = std::filesystem::path((const char8_t*)Filename.c_str()).lexically_normal();
	std::string RangesKey;
	for (ImWchar Char : GlyphRanges) RangesKey += std::format(",{}", (uint32_t)Char);

	std::vector<uint8_t> Data;
	std::string Key;

	if (Gecko::Script* Script = CallingScript())
	{
		std::error_code Error;
		if (std::filesystem::is_directory(Script->ScriptPath, Error))
		{
			std::filesystem::path Path = Script->ScriptPath / Relative;
			if (ReadFile(Path, Data)) Key = (const char*)Path.u8string().c_str();
		}
		else if (std::filesystem::is_regular_file(Script->ScriptPath, Error))
		{
			std::u8string Wanted = Relative.generic_u8string();
			ZipArchive Archive(Script->ScriptPath);
			for (auto& File : Archive)
			{
				// Windows PowerShell's zips use '\\' between folders
				std::u8string Name = File.Filename;
				std::replace(Name.begin(), Name.end(), u8'\\', u8'/');
				if (Name != Wanted) continue;

				Data.assign((const uint8_t*)File.Data.data(), (const uint8_t*)File.Data.data() + File.Data.size());
				Key = (const char*)(Script->ScriptPath.u8string() + u8"|" + Wanted).c_str();
				break;
			}
		}
	}

	if (Key.empty()) ReadFromFontsFolder(Relative, Data, Key);

	if (Key.empty())
	{
		Warn(std::format("Font \"{}\" was not found", Filename));
		return new ScriptFont({}, {}, Size, {});
	}
	if (not IsFontFile(Data))
	{
		Warn(std::format("\"{}\" is not a font", Filename));
		return new ScriptFont({}, {}, Size, {});
	}

	Key += RangesKey;

	if (AtlasFonts.contains(Key)) Data.clear();

	return new ScriptFont(Key, std::move(Data), Size, std::move(GlyphRanges));
}

ScriptFont* ScriptFont::LoadSystem(const std::string& Filename)
{
	PWSTR FontsFolder = nullptr;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &FontsFolder))) return nullptr;
	std::filesystem::path Path = std::filesystem::path(FontsFolder) / std::filesystem::path((const char8_t*)Filename.c_str());
	CoTaskMemFree(FontsFolder);

	std::vector<uint8_t> Data;
	if (not ReadFile(Path, Data) or not IsFontFile(Data)) return nullptr;

	std::string Key = (const char*)Path.u8string().c_str();
	if (AtlasFonts.contains(Key)) Data.clear();

	return new ScriptFont(Key, std::move(Data), 16.0f, {});
}

// Shared by every script and never freed
ScriptFont* ScriptFont::LoadDefault(const wchar_t* Filename)
{
	std::vector<uint8_t> Data;
	std::string Key;
	if (not ReadFromFontsFolder(Filename, Data, Key) or not IsFontFile(Data))
	{
		Warn(std::format("Font \"{}\" was not found in the Fonts folder", std::filesystem::path(Filename).string()));
		Data.clear();
		Key.clear();
	}
	if (AtlasFonts.contains(Key)) Data.clear();

	return new ScriptFont(Key, std::move(Data), 16.0f, {});
}

ScriptFont* ScriptFont::GetDefault()
{
	static ScriptFont* Font = [] { ScriptFont* F = LoadDefault(L"BricolageGrotesque.ttf"); F->AddRef(); return F; }();
	return Font;
}

ScriptFont* ScriptFont::GetDefaultBold()
{
	static ScriptFont* Font = [] { ScriptFont* F = LoadDefault(L"BricolageGrotesqueBold.ttf"); F->AddRef(); return F; }();
	return Font;
}

ScriptFont* ScriptFont::GetDefaultMono()
{
	static ScriptFont* Font = [] { ScriptFont* F = LoadDefault(L"DroidSansMono.ttf"); F->AddRef(); return F; }();
	return Font;
}

ImFont* ScriptFont::Get()
{
	if (Key.empty() or not ::ImGui::GetCurrentContext()) return nullptr;

	auto Found = AtlasFonts.find(Key);
	if (Found != AtlasFonts.end()) return Found->second.Font;
	if (Data.empty()) return nullptr;

	AtlasFont& Entry = AtlasFonts[Key];

	if (not GlyphRanges.empty())
	{
		Entry.GlyphRanges = std::move(GlyphRanges);
		Entry.GlyphRanges.push_back(0);
	}

	void* AtlasData = IM_ALLOC(Data.size());
	memcpy(AtlasData, Data.data(), Data.size());

	ImFontConfig Config;
	Config.GlyphRanges = Entry.GlyphRanges.empty() ? nullptr : Entry.GlyphRanges.data();
	ImFormatString(Config.Name, IM_ARRAYSIZE(Config.Name), "%s", std::filesystem::path((const char8_t*)Key.c_str()).filename().string().c_str());

	Entry.Font = ::ImGui::GetIO().Fonts->AddFontFromMemoryTTF(AtlasData, (int)Data.size(), Size, &Config);
	if (Entry.Font and TwinkUiState::UiMgr) TwinkUiState::UiMgr->MergeIcons();

	Data = {};
	return Entry.Font;
}
