// Precompiled headers.
#include "pch.h"

// Class definition for the TwinkTrackmania class, that also includes extra headers for Trackmania.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Behaviors

TwinkTrackmania::TwinkTrackmania()
{
	this->ExeBaseAddr = (uintptr_t)GetModuleHandleA(NULL);
}

TwinkTrackmania::~TwinkTrackmania()
{
}

// END Behaviors

// Getters

uintptr_t TwinkTrackmania::GetApp()
{
	return Read(uintptr_t, this->ExeBaseAddr + O_APP);
}

uintptr_t TwinkTrackmania::GetViewport()
{
	return Read(uintptr_t, GetApp() + O_M_CTRACKMANIA_VIEWPORT);
}

uintptr_t TwinkTrackmania::GetDirectXDevice()
{
	return Read(uintptr_t, this->ExeBaseAddr + O_D3DDEVICE);
}

#ifdef MANIAPLANET
uintptr_t TwinkTrackmania::GetDirectXContext()
{
	return Read(uintptr_t, this->ExeBaseAddr + O_D3DCONTEXT);
}
#endif

CMwClassInfo* TwinkTrackmania::GetNodClassInfo(uintptr_t Nod)
{
	if (Nod == 0) return nullptr;
	using GetClassInfoFn = CMwClassInfo*(__stdcall*)();
	return reinterpret_cast<GetClassInfoFn>(Virtual(O_V_MWCLASSINFO, Nod))();
}

// END Getters