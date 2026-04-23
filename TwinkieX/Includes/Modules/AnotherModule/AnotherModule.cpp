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

void AnotherModule::RenderInterface()
{
	static char InputField[2048] = { 0 };



	using namespace ImGui;

	Begin("Test");

	InputText("##InputField", InputField, 2048);

	TextTrackmania(InputField);

	End();
}