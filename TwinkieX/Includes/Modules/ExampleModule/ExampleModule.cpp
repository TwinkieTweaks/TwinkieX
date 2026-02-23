#include "pch.h"
#include "ExampleModule.h"

void ExampleModule::RenderMenuMain()
{
	using namespace ImGui;

	if (BeginMenu("Example module"))
	{
		MenuItem("Item");

		BeginDisabled();
		MenuItem("Another item");
		EndDisabled();

		ImGui::EndMenu();
	}
}

void ExampleModule::RenderMenu()
{
	using namespace ImGui;

	if (MenuItem("Open menu", this->Name, MenuOpen))
	{
		MenuOpen = !MenuOpen;
	}
}

void ExampleModule::RenderInterface()
{
	using namespace ImGui;

	if (MenuOpen)
	{
		Begin("Example module");

		Button("Button");

		End();
	}
}