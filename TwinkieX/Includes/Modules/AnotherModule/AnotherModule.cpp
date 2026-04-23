#include "pch.h"
#include "AnotherModule.h"

void AnotherModule::RenderMenuMain()
{
	using namespace ImGui;
	
	if (BeginMenu("Another module"))
	{
		MenuItem("Item");
		
		BeginDisabled();
		MenuItem("Another item");
		EndDisabled();

		// Ooh, fancy item!
		MenuItem("$F00O$F40o$F90h$FD0, $BF0f$7F0a$3F0n$0F1c$0F6y $0FAi$0FFt$0AFe$06Fm$01F!");

		ImGui::EndMenu();
	}
}