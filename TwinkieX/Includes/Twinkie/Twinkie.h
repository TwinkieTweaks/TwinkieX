#pragma once

// Class definition for TwinkTrackmania, used in (this->TrackmaniaMgr)
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Class definition for TwinkUi, used in (this->UiMgr).
#include <Twinkie/TwinkUi/TwinkUi.h>

// The class used by the global Twinkie object (Twinkie gTwinkie).
class Twinkie
{
public:
	// The trackmania manager.
	TwinkTrackmania TrackmaniaMgr;
#ifdef GAMEBOX
	// Usable space
	int pad;
#endif
	// The UI manager.
	TwinkUi UiMgr = TwinkUi(TrackmaniaMgr);

	// Behaviors

	// (__declspec(noinline)) is to prevent the compiler from potentially promoting the function to be inline, raising a warning

	// Ctor
	__declspec(noinline) Twinkie();
	// Dtor
	__declspec(noinline) ~Twinkie();

	// Methods

	// Updates all children
	__declspec(noinline) void Update();

	// Checks if the game is initialized yet
	__declspec(noinline) bool IsGameInit();
};