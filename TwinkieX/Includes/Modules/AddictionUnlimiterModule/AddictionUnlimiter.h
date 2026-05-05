#pragma once
#ifdef TMCN
#include <Modules/IModule.h>

class AddictionUnlimiter : public IModule
{
public:
	AddictionUnlimiter(TwinkTrackmania& Twinkie) : 
		IModule(Twinkie, "AddictionUnlimiter", "AddictionUnlimiter") {
	}

	virtual ~AddictionUnlimiter() override;

	virtual void Render() override;
};
#endif