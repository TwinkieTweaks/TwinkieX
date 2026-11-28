// Precompiled headers.
#include "pch.h"

// Class definition for the Twinkie class used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

#include "Exports/Exports.h"
#include <Version.h>

static bool IsPauseHeld()
{
    return GetAsyncKeyState(VK_PAUSE) < 0;
}

template<typename CheckFn>
static bool WaitForGame(const char* What, CheckFn Check)
{
    auto Start = std::chrono::steady_clock::now();
    if (auto Found = Check())
    {
        LoggertTwink.verbose(LogFormat("{} is ready ({:#x})", What, (uintptr_t)Found));
        return true;
    }

    LoggertTwink.verbose(LogFormat("Waiting for {}", What));
    while (!Check())
    {
        Sleep(1);
        if (IsPauseHeld())
        {
            LoggertTwink.warn(LogFormat("Pause was held while waiting for {}, TwinkieX won't start", What));
            return false;
        }
    }
    LoggertTwink.print(LogFormat("{} is ready after {} ms ({:#x})", What, LogMsSince(Start), (uintptr_t)Check()));
    return true;
}

// This is a separate thread that runs when the DLL is attached.
// It is used to initialize all hooks for the global Twinkie object (Twinkie gTwinkie).
static DWORD WINAPI InitializerThread([[maybe_unused]] LPVOID lpParameter)
{
    auto Start = std::chrono::steady_clock::now();
    LoggertTwink.print(LogFormat("Initializer thread started (thread {})", GetCurrentThreadId()));

    TwinkUiState::UiMgr = &gTwinkie.UiMgr;
    if (IsPauseHeld())
    {
        LoggertTwink.warn(u8"Pause was held at startup, TwinkieX won't start");
        return FALSE;
    }

    // While the game is not initialized, wait
    if (!WaitForGame("the game app", [] { return gTwinkie.TrackmaniaMgr.GetApp(); })) return FALSE;
    if (!WaitForGame("the viewport", [] { return gTwinkie.TrackmaniaMgr.GetViewport(); })) return FALSE;
#ifdef MANIAPLANET
    if (!WaitForGame("the D3D11 swap chain", [] { return gTwinkie.TrackmaniaMgr.GetDirectXSwapChain(); })) return FALSE;
#else
    if (!WaitForGame("the D3D9 device", [] { return gTwinkie.TrackmaniaMgr.GetDirectXDevice(); })) return FALSE;
#endif
#ifdef GAMEBOX
    // Nothing is set up properly when the intro is running, so wait
    if (!WaitForGame("the end of the intro", [] { return gTwinkie.TrackmaniaMgr.GetIsIntroOver(); })) return FALSE;
#endif

    if (IsPauseHeld())
    {
        LoggertTwink.warn(u8"Pause was held, TwinkieX won't start");
        return FALSE;
    }

    LoggertTwink.print(LogFormat("The game is ready after {} ms, initializing", LogMsSince(Start)));

    // After knowing that everything is initialized, we update everything
    gTwinkie.Update();

    LoggertTwink.print(LogFormat("Initialization done in {} ms, the overlay starts on the next frame", LogMsSince(Start)));

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
    {
        wchar_t ExePath[MAX_PATH] = {};
        GetModuleFileNameW(NULL, ExePath, MAX_PATH);
        LoggertTwink.print(LogFormat("TwinkieX {} for " TWINKIEX_GAME " ({}) attached to {} at {:#x}",
            TWINKIEX_VERSION, ILoggert::VerboseEnabled ? "debug build" : "release build",
            std::filesystem::path(ExePath).filename().string(), gTwinkie.TrackmaniaMgr.ExeBaseAddr));
        LoggertTwink.verbose(LogFormat("Exe: {}, DLL at {:#x}, documents: {}", std::filesystem::path(ExePath).string(), (uintptr_t)hModule, gTwinkie.DocumentsFolderPath.string()));
        if (gTwinkie.MinHookStatus != MH_OK) LoggertTwink.error(LogFormat("MinHook failed to initialize: {}", MH_StatusToString(gTwinkie.MinHookStatus)));
        else LoggertTwink.verbose(u8"MinHook initialized");

        // Create the main thread and start it immediately.
        if (!CreateThread(NULL, 0, InitializerThread, NULL, 0, 0)) LoggertTwink.error(LogFormat("Couldn't start the initializer thread: error {}", GetLastError()));
		break;
    }
    
    // These are disabled via (DisableThreadLibraryCalls) up above.
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    // Upon detaching from a process...
    case DLL_PROCESS_DETACH:
        // A non-null (lpReserved) means the process is exiting and every other thread is already dead,
        // so the global destructors must not wait on or release anything those threads owned.
        TwinkUiState::ProcessTerminating = lpReserved != nullptr;
        LoggertTwink.print(TwinkUiState::ProcessTerminating ? u8"Detaching, the game is exiting" : u8"Detaching, TwinkieX is being unloaded");

        if (Veridian::VastVeridian) Veridian::VastVeridian->Save();
        break;
    }

    return TRUE;
}