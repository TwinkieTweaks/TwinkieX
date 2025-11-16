// Precompiled headers.
#include "pch.h"

// Includes class definitions for the Twinkie class, used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

// Behaviors

Twinkie::Twinkie()
{

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

bool Twinkie::IsGameInit()
{
	return this->TrackmaniaMgr.GetApp()
#ifdef MANIAPLANET
		&& this->TrackmaniaMgr.GetViewport() && this->TrackmaniaMgr.GetDirectXSwapChain()
#else
		&& this->TrackmaniaMgr.GetDirectXDevice()
#endif
	;
}

// END Methods