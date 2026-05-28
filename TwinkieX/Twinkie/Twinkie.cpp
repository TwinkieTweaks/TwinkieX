// Precompiled headers.
#include "pch.h"

// Includes class definitions for the Twinkie class, used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

// Behaviors

Twinkie::Twinkie()
{
	Veridian::InitContext(std::filesystem::current_path() / "Twinkie\\Twinkie.ini");

	Veridian::Register("Twinkie", "UiVisible", "Show UI", Veridian::VSettingType::VBool, &TwinkUiState::RenderUi);

	for (IModule*& Module : UiMgr.Modules)
	{
		Veridian::Register("Modules", Module->ID, Module->Name, Veridian::VSettingType::VBool, &Module->Enabled);
	}
}

Twinkie::~Twinkie()
{
#ifdef MANIAPLANET
	ImGui_ImplDX11_Shutdown();
#else
	ImGui_ImplDX9_Shutdown();
#endif
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// END Behaviors

// Methods

void Twinkie::Update()
{
	this->UiMgr.Update();
}

// END Methods