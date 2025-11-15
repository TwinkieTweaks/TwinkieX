#pragma once

// Class definition for TwinkTrackmania, used in (this->TrackmaniaMgr)
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// The class used by the global Twinkie object (Twinkie gTwinkie).
class Twinkie
{
public:

	TwinkTrackmania TrackmaniaMgr;

	// Behaviors

	// (__declspec(noinline)) is to prevent the compiler from potentially promoting the function to be inline, raising a warning that promotes to an error
	__declspec(noinline) Twinkie();
	__declspec(noinline) ~Twinkie();
};