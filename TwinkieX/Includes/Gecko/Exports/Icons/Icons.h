#pragma once
#include <Gecko/ScriptManager.h>

namespace Gecko::Exports::Icons
{
	// Registers the Icons:: and Icons::Kenney:: string constants. Must run after the string type is registered.
	void Registrar(asIScriptEngine* Engine);
}
