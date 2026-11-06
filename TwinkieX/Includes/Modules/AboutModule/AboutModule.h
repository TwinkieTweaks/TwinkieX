#pragma once
#include <Modules/IModule.h>
#include <Version.h>

class AboutModule : public IModule
{
	bool AboutWindowVisible = false;

public:
	AboutModule() = delete;
	AboutModule(TwinkTrackmania& Twinkie, ILoggert& Loggert);

	virtual ~AboutModule();
	virtual void RenderInterface() override;
	virtual void RenderMenuMain() override;
};