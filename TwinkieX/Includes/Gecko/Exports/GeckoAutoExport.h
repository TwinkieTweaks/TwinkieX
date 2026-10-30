#pragma once
#include <Gecko/ScriptManager.h>
#include <iostream>
#include <vector>

namespace Gecko::Exports
{
	enum class AutoExportType { Function, Method, Behaviour, Property, Global };

	struct GeckoExport
	{
		AutoExportType Type;
		const char*    Namespace = "";
		const char*    Class     = nullptr;
		const char*    Decl      = nullptr;
		asSFuncPtr     Ptr;
		asDWORD        CallConv  = 0;
		asEBehaviours  Behaviour = asBEHAVE_CONSTRUCT;
		int            Offset    = 0;
		void*          Address   = nullptr;
	};

	struct GeckoAutoExport { GeckoAutoExport(std::vector<GeckoExport>& L, const GeckoExport& E) { L.push_back(E); }};

	inline void RegisterAll(asIScriptEngine* Engine, const std::vector<GeckoExport>& List)
	{
		for (const auto& E : List)
		{
			Engine->SetDefaultNamespace(E.Namespace);
			switch (E.Type)
			{
			case AutoExportType::Function:  Engine->RegisterGlobalFunction (E.Decl,  E.Ptr,       E.CallConv);                break;
			case AutoExportType::Method:    Engine->RegisterObjectMethod   (E.Class, E.Decl,      E.Ptr,  E.CallConv);        break;
			case AutoExportType::Behaviour: Engine->RegisterObjectBehaviour(E.Class, E.Behaviour, E.Decl, E.Ptr, E.CallConv); break;
			case AutoExportType::Property:  Engine->RegisterObjectProperty (E.Class, E.Decl,      E.Offset);                  break;
			case AutoExportType::Global:    Engine->RegisterGlobalProperty (E.Decl,  E.Address);                              break;
			}
		}
		Engine->SetDefaultNamespace("");
	}
}

#define GECKO_CAT_(a, b) a##b
#define GECKO_CAT(a, b) GECKO_CAT_(a, b)
#define GECKO_AUTOEXPORT(List, ...) \
	static ::Gecko::Exports::GeckoAutoExport GECKO_CAT(GeckoAutoExport_, __COUNTER__){ List, ::Gecko::Exports::GeckoExport{ __VA_ARGS__ } }

#define GECKO_EXPORT_FN(List, Ns_, Decl_, Fn_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Function, .Namespace = Ns_, .Decl = Decl_, .Ptr = asFUNCTION(Fn_), .CallConv = asCALL_CDECL)
#define GECKO_EXPORT_FN_EX(List, Ns_, Decl_, Ptr_, Conv_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Function, .Namespace = Ns_, .Decl = Decl_, .Ptr = Ptr_, .CallConv = Conv_)
#define GECKO_EXPORT_METHOD(List, Ns_, Class_, Decl_, Ptr_, Conv_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Method, .Namespace = Ns_, .Class = Class_, .Decl = Decl_, .Ptr = Ptr_, .CallConv = Conv_)
#define GECKO_EXPORT_BEHAVIOUR(List, Ns_, Class_, Beh_, Decl_, Ptr_, Conv_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Behaviour, .Namespace = Ns_, .Class = Class_, .Decl = Decl_, .Ptr = Ptr_, .CallConv = Conv_, .Behaviour = Beh_)
#define GECKO_EXPORT_PROP(List, Ns_, Class_, Decl_, Offset_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Property, .Namespace = Ns_, .Class = Class_, .Decl = Decl_, .Offset = (int)(Offset_))
#define GECKO_EXPORT_GLOBAL(List, Ns_, Decl_, Addr_) \
	GECKO_AUTOEXPORT(List, .Type = ::Gecko::Exports::AutoExportType::Global, .Namespace = Ns_, .Decl = Decl_, .Address = (void*)(Addr_))
