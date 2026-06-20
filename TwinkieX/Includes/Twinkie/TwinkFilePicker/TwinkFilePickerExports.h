#pragma once

#pragma warning(disable : 4464) // ".." in include path
#pragma warning(push)
#include "../pch.h"
#pragma warning(pop)

#define WM_FILE_SELECTED (WM_APP + 1)

class TwinkFilePicker
{
public:
	enum class FilePickerPurpose
	{
		NoPurpose = -1,
		PickModule = 0,
		PickFont = 1
	};

	bool Done = false;
	FilePickerPurpose Purpose = FilePickerPurpose::NoPurpose;
	std::wstring FinalPath;
	TwinkFilePicker() = delete;
	TwinkFilePicker(FilePickerPurpose Purpose);
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