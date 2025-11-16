#pragma once

// Offsets for various things
#include <Utils/Offsets.h>

// Various types used by Trackmania
#include <Trackmania/TMTypes.h>

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
	uintptr_t ExeBaseAddr = 0;

public:
	// Behaviors

	// Ctor
	__declspec(noinline) TwinkTrackmania();
	// Dtor
	__declspec(noinline) ~TwinkTrackmania();


	// Getters

	// Gets the main app object, usually defined as (CTrackMania*).
	__declspec(noinline) uintptr_t GetApp();

	// Gets the address to the Viewport of the main app
	__declspec(noinline) uintptr_t GetViewport();

	// Gets the class info for a given nod instance.
	__declspec(noinline) CMwClassInfo* GetNodClassInfo(uintptr_t Nod);

	// Gets the game's DirectX device, as LPDIRECT3DDEVICE9 for GAMEBOX, and ID3D11Device for MANIAPLANET
	__declspec(noinline) uintptr_t GetDirectXDevice();

#ifdef MANIAPLANET
	// Gets the game's DirectX swap chain, only for TMCN/D3D11
	__declspec(noinline) uintptr_t GetDirectXSwapChain();
#endif
};