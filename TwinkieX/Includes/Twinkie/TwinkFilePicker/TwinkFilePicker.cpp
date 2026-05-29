#include "pch.h"
#include <Twinkie/TwinkFilePicker/TwinkFilePicker.h>

DWORD WINAPI TwinkFilePickerThread(LPVOID param)
{
    TwinkFilePicker* FilePicker = static_cast<TwinkFilePickerThreadParam*>(param)->FilePicker;
    HWND hWindow = TwinkUiState::Window;

    wchar_t FilePathBuffer[513] = {};

    OPENFILENAMEW OpenFileNameStruct = {};
    OpenFileNameStruct.lStructSize = sizeof(OpenFileNameStruct);
    OpenFileNameStruct.hwndOwner = hWindow;
    OpenFileNameStruct.lpstrFile = FilePathBuffer;
    OpenFileNameStruct.nMaxFile = sizeof(FilePathBuffer) / sizeof(FilePathBuffer[0]);
    OpenFileNameStruct.lpstrFilter =
        L"Dynamic link libraries\0*.DLL\0"
        L"All Files\0*.*\0"
        ;
    OpenFileNameStruct.nFilterIndex = 1;
    OpenFileNameStruct.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&OpenFileNameStruct))
    {
        std::wstring* SelectedPath = new std::wstring(OpenFileNameStruct.lpstrFile);
        TwinkFilePickerWndProcParam* WndProcParam = new TwinkFilePickerWndProcParam{ FilePicker, SelectedPath };
        FilePicker->FinalPath = std::wstring(OpenFileNameStruct.lpstrFile);
        FilePicker->Done = true;

        PostMessageW(
            hWindow,
            WM_FILE_SELECTED,
            0,
            reinterpret_cast<LPARAM>(WndProcParam));
    }

    return 0;
}

TwinkFilePicker::TwinkFilePicker()
{
    TwinkFilePickerThreadParam* ThreadParams = new TwinkFilePickerThreadParam{ this };
    auto hThread = CreateThread(nullptr, 0, TwinkFilePickerThread, ThreadParams, 0, nullptr);

    if (hThread)
    {
        CloseHandle(hThread);
    }
}