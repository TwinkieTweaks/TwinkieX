#pragma once

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
	uintptr_t ExeBaseAddr = 0;

public:
	// Behaviours

	// Ctor
	__declspec(noinline) TwinkTrackmania();
	// Dtor
	__declspec(noinline) ~TwinkTrackmania();


	// Getters

	// Gets the main app object, usually defined as (CTrackMania*).
	uintptr_t GetApp();
};

