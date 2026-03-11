#pragma once
#include <Modules/IModule.h>

// Contains info about a nod. This info is used whenever a nod is right-clicked and a popup shows up
struct PopupNodInfo
{
	const char* Name;
	CMwClassInfo* ClassInfo;
	uintptr_t Nod;
};

class AppExplorerModule : public IModule
{
	// Window visible
	bool Visible = false;
	
	// Last right-clicked nod's info
	PopupNodInfo* NodInfo = nullptr;

	// Nod info popup is visible
	bool NodInfoPopup = false;

public:
	AppExplorerModule(TwinkTrackmania& Twinkie)
		: IModule(Twinkie, "AppExplorer", "AppExplorer") {
	};

	virtual ~AppExplorerModule() = default;

	virtual void RenderInterface() override;
	virtual void RenderMenu() override;

	void RenderNod(uintptr_t Nod, const char* const Name, CMwMemberInfo* NodMemberInfo);
};