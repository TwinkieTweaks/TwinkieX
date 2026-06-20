// Precompiled headers.
#include "pch.h"

// Includes class definitions for the Twinkie class, used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>

// Behaviors

Twinkie::Twinkie()
{
	wchar_t* Buffer = nullptr;
	SHGetKnownFolderPath(FOLDERID_LocalDocuments, 0, NULL, (wchar_t**)&Buffer);

	if (not Buffer) throw;

	DocumentsFolderLocation = Buffer;
	CoTaskMemFree(Buffer);

	Veridian::InitContext(DocumentsFolderLocation / "TwinkieX\\Twinkie.ini");

	Veridian::Register("Twinkie", "UiVisible", "Show UI", Veridian::VSettingType::VBool, &TwinkUiState::RenderUi);

	Veridian::Register("Twinkie", "UiScale", "UI Scale", Veridian::VSettingType::VFloat, &this->UiMgr.UiScale);

	Veridian::Register("Twinkie", "FontPath", "Font path", Veridian::VSettingType::VString, &UiMgr.FontPath, false, [this](Veridian::VSetting*) { 
		ImGui::SameLine();
		if (ImGui::Button("Pick font file"))
		{
			if (this->UiMgr.FilePicker) delete this->UiMgr.FilePicker;
			this->UiMgr.FilePicker = new TwinkFilePicker(TwinkFilePicker::FilePickerPurpose::PickFont);
		}
		ImGui::SameLine();
		if (ImGui::Button("X"))
		{
			this->UiMgr.DestroyCurrentFont = true;
		}
		return true;
		});

	for (IModule*& Module : UiMgr.Modules)
	{
		Veridian::Register("Modules", Module->ID, Module->Name, Veridian::VSettingType::VBool, &Module->Enabled);
	}
}

Twinkie::~Twinkie()
{
#ifdef MANIAPLANET
	ImGui_ImplDX11_Shutdown();
#else
	ImGui_ImplDX9_Shutdown();
#endif
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// END Behaviors

// Methods

void Twinkie::Update()
{
	this->UiMgr.Update();
}

// END Methods