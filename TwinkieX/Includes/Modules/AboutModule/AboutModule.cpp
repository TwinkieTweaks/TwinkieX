#include "pch.h"
#include <Modules/AboutModule/AboutModule.h>

AboutModule::AboutModule(TwinkTrackmania& TrackmaniaMgr)
{
	this->Twinkie = &TrackmaniaMgr;
	this->Name = "About";
	this->ID = "AboutModule";
}

AboutModule::~AboutModule() {}

void AboutModule::RenderInterface()
{
	using namespace ImGui;

	if (not AboutWindowVisible) return;

	if (Begin("About", &AboutWindowVisible, ImGuiWindowFlags_AlwaysAutoResize))
	{
		Text("TwinkieX for " TWINKIEX_GAMEFULL);
		Text(TWINKIEX_VERSION);

		Separator();

		Text("This version of TwinkieX is still $f00work-in-progress$z!");
	}

	End();
}

void AboutModule::RenderMenuMain()
{
	using namespace ImGui;

	if (MenuItem("About", nullptr, AboutWindowVisible))
		AboutWindowVisible = not AboutWindowVisible;
}