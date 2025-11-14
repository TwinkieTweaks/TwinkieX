#pragma once

// Class definition for TwinkTrackmania, used in (this->TrackmaniaMgr)
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// The class used by the global Twinkie object (Twinkie gTwinkie).
class Twinkie
{
public:

	TwinkTrackmania TrackmaniaMgr;

	// Behaviours

	__declspec(noinline) Twinkie();
	__declspec(noinline) ~Twinkie();
};