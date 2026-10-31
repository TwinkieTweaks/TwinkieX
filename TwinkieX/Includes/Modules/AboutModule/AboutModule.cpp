#include "pch.h"
#include "AboutModule.h"
#include <Resources/resource.h>
#include <Modules/ModuleAutoExport.h>
#include <Http/HttpReq.h>

AboutModule::~AboutModule() {}

void AboutModule::RenderInterface()
{
	static HttpRequest Req;
	using namespace ImGui;

	if (not AboutWindowVisible) return;

	if (Begin("About", &AboutWindowVisible, ImGuiWindowFlags_AlwaysAutoResize))
	{
		Text("TwinkieX for " TWINKIEX_GAMEFULL);
		Text(TWINKIEX_VERSION);

		Separator();

		Text("This version of TwinkieX is still \\$f00work-in-progress\\$z!");
	}

	End();
}

void AboutModule::RenderMenuMain()
{
	using namespace ImGui;

	if (MenuItem("About", nullptr, AboutWindowVisible))
		AboutWindowVisible = not AboutWindowVisible;
}

MODULE_EXPORT(AboutModule);