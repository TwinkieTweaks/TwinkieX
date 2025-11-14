// Precompiled headers.
#include "pch.h"

// Offsets for various things
#include <Utils/Offsets.h>

// Class definition for the TwinkTrackmania class.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Behaviours

TwinkTrackmania::TwinkTrackmania()
{
	this->ExeBaseAddr = (uintptr_t)GetModuleHandleA(NULL);
}

TwinkTrackmania::~TwinkTrackmania()
{
}

// END Behaviours

// Getters

uintptr_t TwinkTrackmania::GetApp()
{
	return Read(uintptr_t, this->ExeBaseAddr + O_APP);
}

// END Getters