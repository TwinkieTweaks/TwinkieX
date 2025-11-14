#include "pch.h"
#include "Twinkie/Twinkie.h"

// The global Twinkie object, used to manage all other managers.
Twinkie gTwinkie;

// MainThread is a separate thread that runs when the DLL is attached.
// It is used to initialize all hooks for the global Twinkie object (Twinkie gTwinkie).
static DWORD WINAPI MainThread(LPVOID lpParameter)
{
    // The return value indicates the success/failure of the thread. TRUE is successful.
    return TRUE;
}

// DllMain runs when the DLL is attached/detached.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD CallReason, LPVOID lpReserved)
{
    switch (CallReason)
    {
    case DLL_PROCESS_ATTACH:
        // Disables any subsequent DLL_THREAD_ATTACH/DETATCH CallReasons for DllMain.
        DisableThreadLibraryCalls(hModule);

        // Create the main thread and start it immediately.
        CreateThread(NULL, 0, MainThread, NULL, 0, 0);
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

