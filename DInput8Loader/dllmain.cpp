// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

static HMODULE gRealDInputDll = NULL;
static FARPROC gRealDInput8CreateFn = NULL;

#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8Create@20")
extern "C" __declspec(dllexport) HRESULT __stdcall DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, const void* riidltf, void** ppvOut, void* punkOuter)
{
    if (!gRealDInput8CreateFn)
    {
        if (!gRealDInputDll) gRealDInputDll = LoadLibraryA("C://windows//system32//dinput8.dll");
        if (!gRealDInputDll) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        gRealDInput8CreateFn = GetProcAddress(gRealDInputDll, "DirectInput8Create");
        if (!gRealDInput8CreateFn) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return reinterpret_cast<HRESULT(WINAPI*)(HINSTANCE, DWORD, const void*, void**, void*)>(gRealDInput8CreateFn)(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        LoadLibraryA(TWINKIEX_DLL);
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

