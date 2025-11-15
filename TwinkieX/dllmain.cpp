// Precompiled headers.
#include "pch.h"

// Class definition for the Twinkie class used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

// Included for debugging
#pragma warning(disable : 4710)
#pragma warning(disable : 4711)
#include <iostream>

// The global Twinkie object, used to manage all other managers.
Twinkie gTwinkie;

// This is a separate thread that runs when the DLL is attached.
// It is used to initialize all hooks for the global Twinkie object (Twinkie gTwinkie).
static DWORD WINAPI InitializerThread(LPVOID lpParameter)
{
    // Debugging code
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    // This is just to use the unused (lpParameter) parameter. It is expected to be (NULL).
    if (lpParameter != NULL)
    {
        return FALSE;
    }

    // Debugging code.
    CMwClassInfo* pAppClassInfo = gTwinkie.TrackmaniaMgr.GetNodClassInfo(gTwinkie.TrackmaniaMgr.GetApp());
    CMwClassInfo AppClassInfo = *pAppClassInfo;

    while (pAppClassInfo)
    {
        std::cout << AppClassInfo.ClassName << "\n";
        for (CMwMemberInfo* Member : AppClassInfo)
        {
            std::cout << "\t" << Member->MemberName << " " << Member->MemberOffset << "\n";
        }
        pAppClassInfo = AppClassInfo.ParentClassInfo;
        if (!pAppClassInfo)
        {
            break;
        }
        AppClassInfo = *pAppClassInfo;
    }

    std::cout << "Viewport: " << gTwinkie.TrackmaniaMgr.GetViewport() << "\n";

	std::cout << "DirectX Device: " << std::hex << gTwinkie.TrackmaniaMgr.GetDirectXDevice() << "\n";

#ifdef MANIAPLANET
	std::cout << "DirectX Context: " << std::hex << gTwinkie.TrackmaniaMgr.GetDirectXContext() << "\n";
#endif

    // The return value indicates the success/failure of the thread. TRUE is successful.
    return TRUE;
}

// DllMain runs when the DLL is attached/detached to a process.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD CallReason, LPVOID lpReserved)
{
    // Disables any subsequent (DLL_THREAD_ATTACH/DETACH) CallReasons for DllMain.
    DisableThreadLibraryCalls(hModule);

    switch (CallReason)
    {
    // Upon attaching to a process...
    case DLL_PROCESS_ATTACH:
        // Create the main thread and start it immediately.
        CreateThread(NULL, 0, InitializerThread, NULL, 0, 0);
		break;
    
    // These are disabled via (DisableThreadLibraryCalls) in (case DLL_PROCESS_ATTACH:), so it is okay for them to be wired to (DLL_PROCESS_DETACH) as well.
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

    // Upon detaching from a process...
    case DLL_PROCESS_DETACH:
        break;
    }

	// This is just to use the unused (lpReserved) parameter. It is expected to be (NULL).
    return lpReserved ? TRUE : TRUE;
}

