#include "pch.h"
#include "ScriptFiles.h"
#include "ScriptManager.h"
#include <ZipArchive/ZipArchive.h>
#include <fstream>

namespace Gecko
{
	Script* CallingScript()
	{
		asIScriptContext* Ctx = asGetActiveContext();
		asIScriptFunction* Function = Ctx ? Ctx->GetFunction(0) : nullptr;
		const char* ModuleName = Function ? Function->GetModuleName() : nullptr;
		if (not ModuleName or not TheScriptManager) return nullptr;

		for (auto& Script : TheScriptManager->Scripts)
		{
			if (Script.Name == (const char8_t*)ModuleName) return &Script;
		}
		return nullptr;
	}

	bool ReadScriptFile(const std::filesystem::path& Relative, std::vector<uint8_t>& Out, std::string* Key)
	{
		Script* Script = CallingScript();
		if (not Script) return false;

		std::filesystem::path Normal = Relative.lexically_normal();
		std::error_code Error;

		if (std::filesystem::is_directory(Script->ScriptPath, Error))
		{
			std::filesystem::path Path = Script->ScriptPath / Normal;
			if (not std::filesystem::is_regular_file(Path, Error)) return false;

			std::ifstream File(Path, std::ios::binary);
			if (not File) return false;

			Out.assign(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
			if (Key) *Key = (const char*)Path.u8string().c_str();
			return true;
		}

		if (std::filesystem::is_regular_file(Script->ScriptPath, Error))
		{
			std::u8string Wanted = Normal.generic_u8string();
			ZipArchive Archive(Script->ScriptPath);
			for (auto& File : Archive)
			{
				std::u8string Name = File.Filename;
				std::replace(Name.begin(), Name.end(), u8'\\', u8'/');
				if (Name != Wanted) continue;

				Out.assign((const uint8_t*)File.Data.data(), (const uint8_t*)File.Data.data() + File.Data.size());
				if (Key) *Key = (const char*)(Script->ScriptPath.u8string() + u8"|" + Wanted).c_str();
				return true;
			}
		}

		return false;
	}
}
