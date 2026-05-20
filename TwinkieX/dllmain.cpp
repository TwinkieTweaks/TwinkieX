// Precompiled headers.
#include "pch.h"

// Class definition for the Twinkie class used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

#include "Exports/Exports.h"

// The global Twinkie object, used to manage all other managers.
Twinkie gTwinkie;

// This is a separate thread that runs when the DLL is attached.
// It is used to initialize all hooks for the global Twinkie object (Twinkie gTwinkie).
static DWORD WINAPI InitializerThread(LPVOID lpParameter)
{
    TwinkUiState::UiMgr = &gTwinkie.UiMgr;

    // While the game is not initialized, wait
    while (!gTwinkie.TrackmaniaMgr.GetApp())
    {
        Sleep(1);
    }
    while (!gTwinkie.TrackmaniaMgr.GetViewport())
    {
        Sleep(1);
    }
#ifdef MANIAPLANET
    while (!gTwinkie.TrackmaniaMgr.GetDirectXSwapChain())
    {
        Sleep(1);
    }
#else
    while (!gTwinkie.TrackmaniaMgr.GetDirectXDevice())
    {
        Sleep(1);
    }
#endif
    
    // After knowing that everything is initialized, we update everything
    gTwinkie.Update();

    // This is just to use the unused (lpParameter) parameter. It is expected to be (NULL).
    if (lpParameter != NULL)
    {
        return FALSE;
    }

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
    
    // These are disabled via (DisableThreadLibraryCalls) up above, so it is okay for them to be wired to (DLL_PROCESS_DETACH) as well.
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

    // Upon detaching from a process...
    case DLL_PROCESS_DETACH:
        break;
    }

	// This is just to use the unused (lpReserved) parameter. It is expected to be (NULL).
    return lpReserved ? TRUE : TRUE;
}