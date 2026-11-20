#pragma once
#include <Modules/IModule.h>

extern const char* DashboardStyleNames[];

class InputDisplayModule : public IModule
{
	ImVec4 ColorSteer = ImVec4(0.976f, 0.737f, 0.008f, 1.f);
	ImVec4 ColorSteerI = ImVec4(1.f, 1.f, 1.f, 0.5f);
	ImVec4 ColorAccel = ImVec4(0.f, 0.94f, 0.f, 1.f);
	ImVec4 ColorAccelI = ImVec4(1.f, 1.f, 1.f, 0.5f);
	ImVec4 ColorBrake = ImVec4(0.94f, 0.f, 0.f, 1.f);
	ImVec4 ColorBrakeI = ImVec4(1.f, 1.f, 1.f, 0.5f);
	ImVec4 ColorBackground = ImVec4(0.f, 0.f, 0.f, 0.f);

	std::string StyleName = DashboardStyleNames[0];
	int StyleIdx = 1;

	bool ForceTMVizDimensions = false;

	bool ShowInputDisplay = false;
public:
	InputDisplayModule() = delete;
	InputDisplayModule(TwinkTrackmania&, ILoggert&);

	virtual ~InputDisplayModule();
	virtual void Render() override;
	virtual void RenderMenu() override;

	ImVec2 Lerp(ImVec2 A, ImVec2 B, float T);
};