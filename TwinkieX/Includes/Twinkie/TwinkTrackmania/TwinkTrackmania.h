#pragma once

// Offsets for various things
#include <Utils/Offsets.h>

// Various types used by Trackmania
#include <Trackmania/TMTypes.h>

// Useful for realloc
#include <cstdlib>

#include <type_traits>

#include <cassert>

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
public:
	uintptr_t ExeBaseAddr = 0;

	// Behaviors

	// Ctor
	__declspec(noinline) TwinkTrackmania();
	// Dtor
	__declspec(noinline) ~TwinkTrackmania();


	// Getters

	// Gets the main app object, usually defined as (CTrackMania*).
	__declspec(noinline) CMwNod* GetApp();

	// Gets the address to the Viewport of the main app
	__declspec(noinline) CMwNod* GetViewport();

	// Gets the game's DirectX device, as LPDIRECT3DDEVICE9 for GAMEBOX, and ID3D11Device for MANIAPLANET
	__declspec(noinline) uintptr_t GetDirectXDevice();

	// Gets the game's input port, for mouse and keyboard events (always CDx8InputPort or similar)
	__declspec(noinline) CInputPort* GetInputPort();

	template<typename ReturnT>
	ReturnT* VirtualParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo);

	template<typename ReturnT>
	ReturnT* VirtualParamGet(CMwNod* Nod, uint32_t MemberID, CMwMemberInfo::eType MemberType);

	void PushToStack(CMwStack* Stack, CMwMemberInfo* MemberInfo);

#ifdef MANIAPLANET
	// Gets the game's DirectX swap chain, only for TMCN/D3D11
	__declspec(noinline) uintptr_t GetDirectXSwapChain();
#endif
};

#pragma optimize("", off)
template<typename ReturnT>
ReturnT* TwinkTrackmania::VirtualParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo)
{
	// This variable is used only to not let the compiler optimize the type of Stack->ppMemberInfos for GAMEBOX to a single pointer.
	static CMwStack* PreviousUsedStack = nullptr;

	struct
	{
		ReturnT* pReturnValue;
		ReturnT Storage;
	} Result;

	CMwStack Stack = CMwStack();

	PushToStack(&Stack, MemberInfo);

	PreviousUsedStack = &Stack;

	Nod->VirtualParam_Get(&Stack, &Result);

	return Result.pReturnValue;
}

template<typename ReturnT>
ReturnT* TwinkTrackmania::VirtualParamGet(CMwNod* Nod, uint32_t MemberID, CMwMemberInfo::eType MemberType)
{
	// This variable is used only to not let the compiler optimize the type of Stack->ppMemberInfos for GAMEBOX to a single pointer.
	static CMwStack* PreviousUsedStack = nullptr;

	struct
	{
		ReturnT* pReturnValue;
		ReturnT Storage;
	} Result;
	CMwStack Stack;
	CMwMemberInfo* MemberInfo = nullptr;
	CMwClassInfo* ClassInfo = Nod->MwGetClassInfo();

	while (ClassInfo)
	{
		for (auto& Member : *ClassInfo)
		{
			if (Member->MemberID == MemberID)
			{
				MemberInfo = Member;
				break;
			}
		}
		ClassInfo = ClassInfo->ParentClassInfo;
	}

	if (!MemberInfo) return nullptr;

	PushToStack(&Stack, &MemberInfo);
	
	PreviousUsedStack = &Stack;
	
	Nod->VirtualParam_Get(&Stack, &Result);

	return Result.pReturnValue;
}
#pragma optimize("", on)