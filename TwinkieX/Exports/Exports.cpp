#include "pch.h"
#include "Exports.h"

__declspec(dllexport) void AddModuleToQueue(ModuleCreateFn CreateModuleFn)
{
	gTwinkie.UiMgr.ModuleQueue.push_back(CreateModuleFn(&gTwinkie.TrackmaniaMgr, ImGui::GetCurrentContext()));
}

// This export is used by the launcher to make the game import the DLL automatically (via hijacked import table in dinput8.dll)
__declspec(dllexport) void ThisFunctionDoesNothing()
{
	return;
}