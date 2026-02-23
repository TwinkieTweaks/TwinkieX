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