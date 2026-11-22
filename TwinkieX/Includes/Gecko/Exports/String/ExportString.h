#pragma once
#include <Gecko/ScriptManager.h>

namespace Gecko::Exports::String
{
	std::wstring Utf8ToWide(const std::string& Str);
	std::string WideToUtf8(const std::wstring& Str);

	void RegisterWString(asIScriptEngine* Engine);

	void Registrar(asIScriptEngine* Engine);
}
