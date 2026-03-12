#pragma once
#include <Modules/IModule.h>

// Contains info about a nod. This info is used whenever a nod is right-clicked and a popup shows up
struct PopupNodInfo
{
	const char* Name;
	CMwClassInfo* ClassInfo;
	uintptr_t Nod;
};

// Contains info about a nod's member. This info is used whenever a member is right-clicked and a popup shows up
struct PopupMemberInfo
{
	const char* Name;
	CMwMemberInfo* MemberInfo;
	CMwClassInfo* ParentClassInfo;
	uintptr_t ParentNod;

	// When the member is CLASS-adjacent, this value is non-null and it will be the member itself
	uintptr_t MemberNodItself;
};

class AppExplorerModule : public IModule
{
	// Window visible
	bool Visible = false;
	
	// Last right-clicked nod's info
	PopupNodInfo* NodInfo = nullptr;

	// Nod info popup is visible
	bool NodInfoPopup = false;

	// Last right-clicked member's info
	PopupMemberInfo* MemberInfo = nullptr;

	// Member info popup is visible
	bool MemberInfoPopup = false;

	// Tracks if the window position of any popup was set to the mouse cursor or not
	bool HasSetWindowPositionForPopup = false;

public:
	AppExplorerModule(TwinkTrackmania& Twinkie)
		: IModule(Twinkie, "AppExplorer", "AppExplorer") {
	};

	virtual ~AppExplorerModule() = default;

	virtual void RenderInterface() override;
	virtual void RenderMenu() override;

	void RenderNod(uintptr_t Nod, const char* const Name, CMwMemberInfo* NodMemberInfo);
	void RenderNodInfoClassInfo(CMwClassInfo* ClassInfo);
	void RenderNodInfoMemberInfo();

	void SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const char* NodName, uintptr_t Nod, CMwClassInfo* ClassInfo);
	void SetClassInfoPopup(CMwClassInfo* ClassInfo, const char* NodName, uintptr_t Nod);
};