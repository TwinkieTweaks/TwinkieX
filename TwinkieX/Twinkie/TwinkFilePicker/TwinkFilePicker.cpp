#include "pch.h"
#include <Twinkie/TwinkFilePicker/TwinkFilePicker.h>

DWORD WINAPI TwinkFilePickerThread(LPVOID param)
{
    using enum TwinkFilePicker::FilePickerPurpose;

    auto ThreadParam = static_cast<TwinkFilePickerThreadParam*>(param);
    TwinkFilePicker* FilePicker = ThreadParam->FilePicker;
    HWND hWindow = TwinkUiState::Window;

    wchar_t FilePathBuffer[513] = {};

    OPENFILENAMEW OpenFileNameStruct = {};
    OpenFileNameStruct.lStructSize = sizeof(OpenFileNameStruct);
    OpenFileNameStruct.hwndOwner = hWindow;
    OpenFileNameStruct.lpstrFile = FilePathBuffer;
    OpenFileNameStruct.nMaxFile = sizeof(FilePathBuffer) / sizeof(FilePathBuffer[0]);

    switch (FilePicker->Purpose)
    {
        case PickModule:
        {
            OpenFileNameStruct.lpstrFilter =
                L"Dynamic link libraries\0*.DLL\0"
                L"All Files\0*.*\0"
                ;
            break;
        }
        case PickFont:
        {
            OpenFileNameStruct.lpstrFilter =
                L"TrueType Font file\0*.TTF\0"
                L"All Files\0*.*\0"
                ;
            OpenFileNameStruct.lpstrInitialDir = ThreadParam->TwinkieFontsPath.c_str();
            break;
        }
        case NoPurpose:
        {
            OpenFileNameStruct.lpstrFilter =
                L"All Files\0*.*\0"
                ;
            break;
        }
    }

    OpenFileNameStruct.nFilterIndex = 1;
    OpenFileNameStruct.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

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

    SetCurrentDirectory(ThreadParam->TwinkieFontsPath.remove_filename().c_str());

    return 0;
}

TwinkFilePicker::TwinkFilePicker(FilePickerPurpose Purpose, std::filesystem::path TwinkieFontsPath) : Purpose(Purpose)
{
    TwinkFilePickerThreadParam* ThreadParams = new TwinkFilePickerThreadParam{ this, TwinkieFontsPath };
    auto hThread = CreateThread(nullptr, 0, TwinkFilePickerThread, ThreadParams, 0, nullptr);

    if (hThread)
    {
        CloseHandle(hThread);
    }
}