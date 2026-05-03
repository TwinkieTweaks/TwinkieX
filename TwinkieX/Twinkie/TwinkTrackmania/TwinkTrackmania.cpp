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

CMwNod* TwinkTrackmania::GetApp()
{
#ifndef TM1
	return ReadAddr(CMwNod*, this->ExeBaseAddr + O_APP);
#else
	return ReadAddr(CMwNod*, O_M_CMWNODMAIN_APP + O_APP);
#endif
}

CMwNod* TwinkTrackmania::GetViewport()
{
#ifndef TM1
	return ReadAddr(CMwNod*, (uintptr_t)GetApp() + O_M_CTRACKMANIA_VIEWPORT);
#else
	return ReadAddr(CMwNod*, O_M_CMWNODMAIN_VIEWPORT + O_APP);
#endif
}

uintptr_t TwinkTrackmania::GetDirectXDevice()
{
#ifndef TM1
	return ReadAddr(uintptr_t, this->ExeBaseAddr + O_D3DDEVICE);
#else
	return ReadAddr(uintptr_t, O_M_CVISIONVIEWPORTDX9_DEVICE + (uintptr_t)GetViewport());
#endif
}

CInputPort* TwinkTrackmania::GetInputPort()
{
	return ReadAddr(CInputPort*, (uintptr_t)GetApp() + O_M_CTRACKMANIA_INPUTPORT);
}

#ifdef MANIAPLANET
uintptr_t TwinkTrackmania::GetDirectXSwapChain()
{
	return ReadAddr(uintptr_t, (uintptr_t)this->GetViewport() + O_M_CDX11VIEWPORT_D3DSWAPCHAIN);
}
#endif

void TwinkTrackmania::PushToStack(CMwStack* Stack, CMwMemberInfo* MemberInfo)
{
#ifdef GAMEBOX
	if (Stack->m_Size != 0 and Stack->ppMemberInfos)
	{
		realloc(Stack->ppMemberInfos, (Stack->m_Size + 1) * sizeof(CMwMemberInfo*));

		Stack->ppMemberInfos[Stack->m_Size] = MemberInfo;

		Stack->m_Size++;
	}
	else
	{
		Stack->ppMemberInfos = new CMwMemberInfo* { MemberInfo };
		Stack->m_Size = 1;
	}
#else
	if (Stack->m_Size < 2)
	{
		Stack->m_ContainedItems[Stack->m_Size] = CMwStack::Item{ MemberInfo, CMwStack::ITEM_MEMBER };
		Stack->m_Size++;
	}
	else
	{
		if (Stack->m_ExtraItemsCapacity != 0 and Stack->m_pExtraItems)
		{
			realloc(Stack->m_pExtraItems, (Stack->m_ExtraItemsCapacity + 1) * sizeof(CMwStack::Item));

			Stack->m_pExtraItems[Stack->m_ExtraItemsCapacity] = CMwStack::Item{ MemberInfo, CMwStack::ITEM_MEMBER };
			Stack->m_ExtraItemsCapacity++;
			Stack->m_Size++;
		}
	}
#endif
}

// END Getters