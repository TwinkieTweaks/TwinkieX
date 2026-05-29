#pragma once

#pragma warning(disable : 4464) // ".." in include path
#pragma warning(push)
#include "../pch.h"
#pragma warning(pop)

#define WM_FILE_SELECTED (WM_APP + 1)

class TwinkFilePicker
{
public:
	bool Done = false;
	std::wstring FinalPath;
	TwinkFilePicker();
};

struct TwinkFilePickerThreadParam
{
    TwinkFilePicker* FilePicker;
};

struct TwinkFilePickerWndProcParam
{
    TwinkFilePicker* FilePicker;
    std::wstring* Path;
};