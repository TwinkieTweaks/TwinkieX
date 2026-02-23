#pragma once
#include "../IModule.h"

class ExampleModule : public IModule
{
public:
	bool MenuOpen = false;

	ExampleModule(TwinkTrackmania& Twinkie)
		: IModule(Twinkie, "ExampleModule", "Example module") {
	};

	virtual ~ExampleModule() = default;

	virtual void RenderMenuMain() override;
	virtual void RenderMenu() override;
	virtual void RenderInterface() override;
};