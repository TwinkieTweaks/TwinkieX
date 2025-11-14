#pragma once

// Precompiled headers, but this is for Intellisense.
#include "../../pch.h"

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
	uintptr_t ExeBaseAddr = 0;

public:
	// Behv

	// Ctor
	TwinkTrackmania();
	// Dtor
	~TwinkTrackmania();


	// Getters

	// Gets the main app object, usually defined as (CTrackMania*).
	uintptr_t GetApp();
};

