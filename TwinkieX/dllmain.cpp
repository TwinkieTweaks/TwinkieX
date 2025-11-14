// Precompiled headers.
#include "pch.h"

// Class definition for the Twinkie class used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

// The global Twinkie object, used to manage all other managers.
Twinkie gTwinkie;

// MainThread is a separate thread that runs when the DLL is attached.
// It is used to initialize all hooks for the global Twinkie object (Twinkie gTwinkie).
static DWORD WINAPI InitializerThread(LPVOID lpParameter)
{
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
    switch (CallReason)
    {
    // Upon attaching to a process...
    case DLL_PROCESS_ATTACH:
        // Disables any subsequent DLL_THREAD_ATTACH/DETACH CallReasons for DllMain.
        DisableThreadLibraryCalls(hModule);

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
    return lpReserved ? TRUE : TRUE;
}

