#pragma once
#include <Modules/IModule.h>

class AnotherModule : public IModule
{
public:
	AnotherModule(TwinkTrackmania& Twinkie)
		: IModule(Twinkie, "AnotherModule", "Another module") {
		
	};

	virtual ~AnotherModule() = default;

	virtual void RenderMenuMain() override;
};