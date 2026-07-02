// Precompiled headers.
#include "pch.h"

// Includes class definitions for the Twinkie class, used by (Twinkie gTwinkie).
#include <Twinkie/Twinkie.h>
#include <DiscordGameSDK/DiscordStuff.h>

// The global Twinkie object, used to manage all other managers.
Twinkie gTwinkie;

// Behaviors

Twinkie::Twinkie()
{
	wchar_t* Buffer = nullptr;
	SHGetKnownFolderPath(FOLDERID_LocalDocuments, 0, NULL, (wchar_t**)&Buffer);

	if (not Buffer) throw;

	DocumentsFolderPath = Buffer;
	CoTaskMemFree(Buffer);

	Veridian::InitContext(DocumentsFolderPath / "TwinkieX\\Twinkie.ini");

	Veridian::Register("Twinkie", "UiVisible", "Show UI", Veridian::VSettingType::VBool, &TwinkUiState::RenderUi);

	Veridian::Register("Twinkie", "UiScale", "UI Scale", Veridian::VSettingType::VFloat, &this->UiMgr.UiScale);

	Veridian::Register("Twinkie", "FontPath", "Font path", Veridian::VSettingType::VString, &UiMgr.FontPath, false, [this](Veridian::VSetting*) { 
		ImGui::SameLine();
		if (ImGui::Button("Pick font file"))
		{
			if (this->UiMgr.FilePicker) delete this->UiMgr.FilePicker;
			this->UiMgr.FilePicker = new TwinkFilePicker(TwinkFilePicker::FilePickerPurpose::PickFont, DocumentsFolderPath / "TwinkieX\\Fonts");
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
	delete Veridian::VastVeridian;

#ifdef MANIAPLANET
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#else
	ImGui_ImplDX9_Shutdown();

	// PLEASE NOTE: DX9 shutdown IMMEDIATELY shuts down the current process. ANY CODE PAST THIS POINT WILL NOT BE RUN ON DX9.

	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

// END Behaviors

// Methods

void Twinkie::Update()
{
	this->UiMgr.Update(DocumentsFolderPath);
}

// END Methods