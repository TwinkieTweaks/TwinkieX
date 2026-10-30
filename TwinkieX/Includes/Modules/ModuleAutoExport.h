#pragma once
#include <Twinkie/Twinkie.h>
#include <iostream>
#include <vector>

using ModuleCreateFn = IModule * (*)(TwinkTrackmania*, ILoggert*);
extern std::vector<ModuleCreateFn> ModuleCreationFns;
struct ModuleAutoExport { ModuleAutoExport(ModuleCreateFn E) { ModuleCreationFns.push_back(E); } };

inline void CreateAndExportModules(std::vector<IModule*>& Modules, TwinkTrackmania& TrackmaniaMgr, ILoggert* Loggert)
{
	for (auto& Fn : ModuleCreationFns)
	{
		Modules.push_back(Fn(&TrackmaniaMgr, Loggert));
	}
}

#define MODULE_CAT_(a, b) a##b
#define MODULE_CAT(a, b) MODULE_CAT_(a, b)
#define MODULE_EXPORTFN(Module) \
	static IModule* MODULE_CAT(ModuleExportFn_, Module)(TwinkTrackmania* pTwinkie, ILoggert* Loggert) { return static_cast<IModule*>(new Module(*pTwinkie, *Loggert)); }
#define MODULE_EXPORTFNDECL(Module) \
	MODULE_CAT(ModuleExportFn_, Module)
#define MODULE_AUTOEXPORT(Module) \
	static ModuleAutoExport MODULE_CAT(ModuleAutoExport_, __COUNTER__){ MODULE_EXPORTFNDECL(Module) }
#define MODULE_EXPORT(Module) \
	MODULE_EXPORTFN(Module) MODULE_AUTOEXPORT(Module) 