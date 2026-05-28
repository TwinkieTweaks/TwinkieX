#pragma once

// Class definition for TwinkTrackmania, used in (this->TrackmaniaMgr)
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Class definition for TwinkUi, used in (this->UiMgr).
#include <Twinkie/TwinkUi/TwinkUi.h>

// The Veridian settings manager
#include <Veridian/Veridian.h>

// The Win32/DX9/DX11 implementations for ImGui
#include <imgui/imgui_impl_win32.h>
#ifdef MANIAPLANET
#include <imgui/imgui_impl_dx11.h>
#else
#include <imgui/imgui_impl_dx9.h>
#endif
// The class used by the global Twinkie object (Twinkie gTwinkie).
class Twinkie
{
public:
	// The trackmania manager.
	TwinkTrackmania TrackmaniaMgr;

	// The UI manager.
	TwinkUi UiMgr = TwinkUi(TrackmaniaMgr);

	// Behaviors

	// Ctor
	Twinkie();
	// Dtor
	~Twinkie();

	// Methods

	// Updates all children
	void Update();
};