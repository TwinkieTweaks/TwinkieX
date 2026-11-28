#pragma once
#include "Arch.h"
#include "Stdlibs.h"

#define TWINKIEX_API __declspec(dllexport)

extern DWORD ProtFlags;
extern const char* g_MemberTypeNames[];
extern const char* g_MemberTypeSignatures[];
enum class EGame
{
    // TM1
    Trackmania1,
    // TMO
    TrackmaniaOriginal,
    // TMS
    TrackmaniaSunrise,
    // TMSX
    TrackmaniaSunriseExtreme,
    // ESWC
    TrackmaniaNationsESWC,
    // TMU
    TrackmaniaUnited,
    // TMCN
    TrackmaniaChina
};

// Useful definitions

// Used to read from an address
#define ReadAddr(type, addr) (*(type*)((addr)))

// Used to write to an address
#define WriteAddr(type, addr, value) *(type*)((addr)) = (value)

// Used to get the n-th virtual function from an object ptr
#define Virtual(n, obj) (*(uintptr_t**)(obj))[n]

#define VirtualPtr(n, obj) &(Virtual((n), (obj)))

// Unprotects memory at address with size
#define Unprotect(addr, size) VirtualProtect((void*)(addr), (size), PAGE_EXECUTE_READWRITE, &ProtFlags)

#define OffsetOf(Class, member) (uintptr_t)&(((Class*)(0))->Member)

#define DeleteImplOperatorEq(Class) Class operator=(Class) = delete

// Used to write to the n-th virtual function from an object ptr
__declspec(noinline) uintptr_t VirtualWrite(unsigned int Idx, uintptr_t This, uintptr_t ToWrite);
__declspec(noinline) bool IsAddrSafeSlow(uintptr_t Addr);

// Offsets
// Unset values must be zero, unless stated that they are set

#if defined(TMCN)

// Globals
#define O_APP 0x1C7AB90
#define O_D3DDEVICE 0x1C7AB20
#define O_ENGINEMGR 0x1c89e08

#define O_FN_CMWCMDFASTCALLSTATICPARAM_CREATE 0x28e500
#define O_FN_CMWNOD_RELEASE 0x278d00
#define O_FN_CSYSTEMARCHIVENOD_LOADGBX 0x63f590

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 6
#define O_CMDBUFFER_UPDATE_MAINLOOP 13
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 14
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 15
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 16
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 17
#define O_CMDBUFFER_UPDATESCENEENGINE 22

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 96
#define O_M_CTRACKMANIA_INPUTPORT 0x70

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x178
#define O_M_CINPUTPORT_ISFOCUSED 0xA98

#define O_M_CINPUTDEVICE_ISDISABLED 0xE4

#define O_M_CMWNOD_REFCOUNT 0x10

// Exclusive to TMCN, the other games have the device as a global
#define O_M_CDX11VIEWPORT_D3DSWAPCHAIN 8808

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_PRESENT 8
#define O_V_RESIZEBUFFERS 13
#define O_V_CMWCMD_START 33
#define O_V_CMWCMD_STOP 34

#elif defined(TM1)

// Globals

// NOTE: For TM1.0, "APP" refers to the global object of type CMwNodMain*.
#define O_APP 0x97b108

// NOTE: Null on purpose for TM1.0. Get the DirectX device from the viewport instead.
#define O_D3DDEVICE 0x0

#define O_ISINTROOVER 0x5159F4
#define O_ENGINEMGR 0x580494

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 21
#define O_CMDBUFFER_UPDATE_MAINLOOP 22
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 22
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 23
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 23
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 23
#define O_CMDBUFFER_UPDATESCENEENGINE 29

// Offsets to members

// Exclusive to TM1.0, since the app is not a global but a child of a global CMwNodMain*.
#define O_M_CMWNODMAIN_APP 0x38

// Exclusive to TM1.0, since the viewport is not a global but a child of a global CMwNodMain*.
#define O_M_CMWNODMAIN_VIEWPORT 0x40

// Exclusive to TM1.0, because the DirectX device is not a global.
#define O_M_CVISIONVIEWPORTDX9_DEVICE 0x64C

// NOTE: Null on purpose for TM1.0. Get the viewport from CMwNodMain* instead.
#define O_M_CTRACKMANIA_VIEWPORT 0x0
// NOTE: CTRACKMANIA here is inaccurate for TM1.0. The real type is CGameProcess*
#define O_M_CTRACKMANIA_INPUTPORT 0x40

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28
#define O_M_CINPUTPORT_ISFOCUSED 0x28 + 16

#define O_M_CINPUTDEVICE_ISDISABLED 0x0 

#define O_M_CMWNOD_REFCOUNT 0x4
#define O_SIZE_CMWCMDFASTCALL 0x24

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// Vtable indices
#define O_V_RESET 16
#define O_V_PRESENT 17

#elif defined(TMS)

// Globals
#define O_APP 0x0
#define O_D3DDEVICE 0x0
#define O_ISINTROOVER 0x0

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0
#define O_M_CINPUTPORT_ISFOCUSED 0

#define O_M_CINPUTDEVICE_ISDISABLED 0x0

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_PRESENT 0

#elif defined(TMSX)

// Globals
#define O_APP 0x7CCE50
#define O_D3DDEVICE 0x7AA3BC
#define O_ISINTROOVER 0x78BE8C
#define O_ENGINEMGR 0x78ABD0

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 5
#define O_CMDBUFFER_UPDATE_MAINLOOP 12
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 13
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 14
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 15
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 16
#define O_CMDBUFFER_UPDATESCENEENGINE 21

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 80
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28
#define O_M_CINPUTPORT_ISFOCUSED 0x28 + 16

#define O_M_CINPUTDEVICE_ISDISABLED 0x0

#define O_M_CMWNOD_REFCOUNT 0x4
#define O_SIZE_CMWCMDFASTCALL 0x24

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_RESET 16
#define O_V_PRESENT 17

#elif defined(TMO)

// Globals
#define O_APP 0x7C8084
#define O_D3DDEVICE 0x7A4AAC
#define O_ISINTROOVER 0x7A4B04
#define O_ENGINEMGR 0x785E30

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 5
#define O_CMDBUFFER_UPDATE_MAINLOOP 12
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 13
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 14
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 15
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 16
#define O_CMDBUFFER_UPDATESCENEENGINE 21

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 80
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28
#define O_M_CINPUTPORT_ISFOCUSED 0x28 + 16

#define O_M_CINPUTDEVICE_ISDISABLED 0x0

#define O_M_CMWNOD_REFCOUNT 0x4
#define O_SIZE_CMWCMDFASTCALL 0x24

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_RESET 16
#define O_V_PRESENT 17

#elif defined(ESWC)

// Globals
#define O_APP 0x8AEE50
#define O_D3DDEVICE 0x8B0DCC
#define O_ISINTROOVER 0x8BE1D4
#define O_ENGINEMGR 0x8960E4

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 6
#define O_CMDBUFFER_UPDATE_MAINLOOP 13
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 14
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 15
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 16
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 17
#define O_CMDBUFFER_UPDATESCENEENGINE 22

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28
#define O_M_CINPUTPORT_ISFOCUSED 0x28 + 16

#define O_M_CINPUTDEVICE_ISDISABLED 0x0

#define O_M_CMWNOD_REFCOUNT 0x4
#define O_SIZE_CMWCMDFASTCALL 0x24

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_RESET 16
#define O_V_PRESENT 17

#elif defined(TMU)

// Globals
#define O_APP 0x82CD60
#define O_D3DDEVICE 0x8237AC
// TMU has no intro
#define O_ISINTROOVER 0x0
#define O_ENGINEMGR 0x826C5C

#define O_CMDBUFFER_ALLSCRIPTS_BEFORE 6
#define O_CMDBUFFER_UPDATE_MAINLOOP 13
#define O_CMDBUFFER_UPDATE_GAMEAPPASYNC 14
#define O_CMDBUFFER_UPDATE_AFTERMAINLOOP 15
#define O_CMDBUFFER_NETWORK_AFTERMAINLOOP 16
#define O_CMDBUFFER_ALLSCRIPTS_AFTER 17
#define O_CMDBUFFER_UPDATESCENEENGINE 22

// Offsets to members
#define O_M_CTRACKMANIA_VIEWPORT 0
#define O_M_CTRACKMANIA_INPUTPORT 0x58

#define O_M_CINPUTPORT_CONNECTEDDEVICES 0x28
#define O_M_CINPUTPORT_ISFOCUSED 0x28 + 16

#define O_M_CINPUTDEVICE_ISDISABLED 0x0

#define O_M_CMWNOD_REFCOUNT 0x4
#define O_SIZE_CMWCMDFASTCALL 0x24

// Member IDs
#define O_MEMBERID_CGAMEAPP_VIEWPORT 0x03005002
#define O_MEMBERID_CGAMEAPP_INPUTPORT 0x03005004

// MwId name table
#define O_MWID_MAXCHUNKS 32

// Vtable indices
#define O_V_RESET 16
#define O_V_PRESENT 17

#endif