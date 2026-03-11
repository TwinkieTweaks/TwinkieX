#pragma once
#include "Arch.h"
#include "Stdlibs.h"

extern DWORD ProtFlags;

// Useful definitions

// Used to read from an address
#define ReadAddr(type, addr) (*(type*)((addr)))

// Used to write to an address
#define WriteAddr(type, addr, value) *(type*)((addr)) = (value)

// Used to get the n-th virtual function from an object ptr
#define Virtual(n, obj) (*(uintptr_t**)(obj))[n]

#define VirtualPtr(n, obj) &(Virtual((n), (obj)))

// Unprotects memory at address with size
#define Unprotect(addr, size) VirtualProtect((LPVOID)(addr), (size), PAGE_EXECUTE_READWRITE, &ProtFlags)

// Used to write to the n-th virtual function from an object ptr
__declspec(noinline) uintptr_t VirtualWrite(unsigned int Idx, uintptr_t This, uintptr_t ToWrite);

// Offsets
// Unset values must be zero, unless stated that they are set

#if defined(TMCN)

// Globals
#define O_APP 0x1C7AB90
#define O_D3DDEVICE 0x1C7AB20

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 96
#define O_M_CTRACKMANIA_INPUTPORT 0x70

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x178

// Exclusive to TMCN, the other games have the device as a global
#define O_M_CDX11VIEWPORT_D3DSWAPCHAIN 8808

// Vtable indices
#define O_V_MWCLASSINFO 2
#define O_V_PRESENT 8

#elif defined(TM1)

// Globals
#define O_APP 0x0
#define O_D3DDEVICE 0x0

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0

// Vtable indices
#define O_V_MWCLASSINFO 0
#define O_V_PRESENT 17

#elif defined(TMS)

// Globals
#define O_APP 0x0
#define O_D3DDEVICE 0x0

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0

// Vtable indices
#define O_V_MWCLASSINFO 0
#define O_V_PRESENT 0

#elif defined(TMSX)

// Globals
#define O_APP 0x7CCE50
#define O_D3DDEVICE 0x7AA3BC

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 80
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28

// Vtable indices
#define O_V_MWCLASSINFO 1
#define O_V_PRESENT 17

#elif defined(TMO)

// Globals
#define O_APP 0x7C8084
#define O_D3DDEVICE 0x7A4AAC

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 80
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28

// Vtable indices
#define O_V_MWCLASSINFO 1
#define O_V_PRESENT 17

#elif defined(ESWC)

// Globals
#define O_APP 0x8AEE50
#define O_D3DDEVICE 0x8B0DCC

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28

// Vtable indices
#define O_V_MWCLASSINFO 1
#define O_V_PRESENT 17

#endif