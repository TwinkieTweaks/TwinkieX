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

		ImGui::EndMenu();
	}
}