#pragma once
#ifdef TMCN 
#include <Modules/IModule.h>

class AddictionUnlimiter : public IModule
{
public:
	AddictionUnlimiter(TwinkTrackmania& Twinkie)
	{
		this->Twinkie = &Twinkie;
		this->Name = "AddictionUnlimiter";
		this->ID = "AddictionUnlimiter";
	};

	virtual ~AddictionUnlimiter() override;

	virtual void Render() override;
};
#endif