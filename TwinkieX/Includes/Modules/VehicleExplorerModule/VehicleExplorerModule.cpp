#include "pch.h"
#ifdef TMCN
#include <Modules/VehicleExplorerModule/VehicleExplorerModule.h>
#include <format>

#define CopyButton(Thing) if (SmallButton("Copy")) { TempPtrBuffer = PtrToString(Thing); SetClipboardText(TempPtrBuffer.c_str()); }
#define PreviewPtr(Thing) PushID(Thing); Text(#Thing ": %p", Thing); SameLine(); CopyButton(Thing); PopID();


VehicleExplorer::~VehicleExplorer() {}

void VehicleExplorer::Render()
{
	auto PtrToString = [](void* Ptr) { return std::format("{:08x}", Ptr); };

	using namespace ImGui;

	auto Playground = Twinkie->ParamGet<CMwNod*>(Twinkie->GetApp(), "CurrentPlayground");
	
	Begin("VehicleExplorer");

	if (not Playground)
	{
		Text("No playground.");
		End();
		return;
	}
	
	auto Arena = Twinkie->ParamGet<CMwNod*>(Playground, "Arena");
	if (not Arena)
	{
		Text("No arena.");
		End();
		return;
	}

	auto Players = Twinkie->ParamGet<CFastBuffer<CMwNod*>>(Arena, "Players");
	if (not Players)
	{
		Text("No players.");
		End();
		return;
	}

	SeparatorText("Pointers");
	{
		std::string TempPtrBuffer;	
		
		for (auto& Player : *Players)
		{
			if (not Player) continue;

			PreviewPtr(Player);
			
			CMwNod* Vehicle = ReadAddr(CMwNod*, ((uintptr_t)Player) + 0x9F0);

			PreviewPtr(Vehicle);
		}
	}

	SeparatorText("Memory explore");
	{
		static uint64_t Address = 0;
		static uint64_t Offset  = 0;

		template for () {}
	}

	End();
}
#endif