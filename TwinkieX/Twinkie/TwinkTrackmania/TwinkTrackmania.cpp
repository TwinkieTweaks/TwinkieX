// Precompiled headers.
#include "pch.h"
#include "../../pch.h"

// Offsets for various things
#include "../../Utils/Offsets.h"

// Class definition for the TwinkTrackmania class.
#include "TwinkTrackmania.h"

// Behv

TwinkTrackmania::TwinkTrackmania()
{
	this->ExeBaseAddr = (uintptr_t)GetModuleHandleA(NULL);
}

TwinkTrackmania::~TwinkTrackmania()
{
}

// END Behv

// Getters

uintptr_t TwinkTrackmania::GetApp()
{
	return Read(uintptr_t, this->ExeBaseAddr + O_APP);
}

// END Getters