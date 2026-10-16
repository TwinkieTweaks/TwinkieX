#pragma once

#include <Twinkie/Twinkie.h>
#include <Twinkie/TwinkLogs/CppLoggert/CppLoggert.h>
#include <imgui/imgui.h>

using ModuleCreateFn = IModule * (*)(TwinkTrackmania*, ImGuiContext*, ILoggert*);

extern Twinkie gTwinkie;

extern __declspec(dllexport) void AddModuleToQueue(ModuleCreateFn CreateModuleFn);