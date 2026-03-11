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

CMwClassInfo* TwinkTrackmania::GetNodClassInfo(uintptr_t Nod)
{
	using MwClassInfoFn = CMwClassInfo*(__thiscall*)(uintptr_t);

	return reinterpret_cast<MwClassInfoFn>(Virtual(O_V_MWCLASSINFO, Nod))(Nod);
}

uintptr_t TwinkTrackmania::GetApp()
{
	return ReadAddr(uintptr_t, this->ExeBaseAddr + O_APP);
}

uintptr_t TwinkTrackmania::GetViewport()
{
	return ReadAddr(uintptr_t, GetApp() + O_M_CTRACKMANIA_VIEWPORT);
}

uintptr_t TwinkTrackmania::GetDirectXDevice()
{
	return ReadAddr(uintptr_t, this->ExeBaseAddr + O_D3DDEVICE);
}

CInputPort* TwinkTrackmania::GetInputPort()
{
	return ReadAddr(CInputPort*, GetApp() + O_M_CTRACKMANIA_INPUTPORT);
}

#ifdef MANIAPLANET
uintptr_t TwinkTrackmania::GetDirectXSwapChain()
{
	return ReadAddr(uintptr_t, this->GetViewport() + O_M_CDX11VIEWPORT_D3DSWAPCHAIN);
}
#endif

// END Getters