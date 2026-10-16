#pragma once
#ifdef TMCN 
#include <Modules/IModule.h>

// TODO: Nuke this. Something better is in TwinkTrackmania ctor.
class AddictionUnlimiter : public IModule
{
public:
	AddictionUnlimiter(TwinkTrackmania& Twinkie, ILoggert& Loggert) : IModule{ Twinkie, Loggert, "AddictionUnlimiter", "AddictionUnlimiter" } {};

	virtual ~AddictionUnlimiter() override;

	virtual void Render() override;
};
#endif