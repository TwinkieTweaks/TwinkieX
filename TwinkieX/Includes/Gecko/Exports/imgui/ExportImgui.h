#pragma once
#include <Gecko/ScriptManager.h>

namespace Gecko::Exports::ImGui
{
	void Registrar(asIScriptEngine* Engine);

	// The UI:: enums (in ExportImguiEnums.cpp)
	void RegisterEnums(asIScriptEngine* Engine);
}