#pragma once
#include <Modules/IModule.h>
#include <string>

constexpr bool IsMemberClassAdjacent(CMwMemberInfo* Member);

// Contains info about a nod. This info is used whenever a nod is right-clicked and a popup shows up
struct PopupNodInfo
{
	const std::string Name = "";
	CMwClassInfo* ClassInfo = nullptr;
	uintptr_t Nod = 0;

	PopupNodInfo() = delete;
	PopupNodInfo(PopupNodInfo&) = delete;
	PopupNodInfo& operator=(const PopupNodInfo&) = delete;

	PopupNodInfo(const std::string Name) : Name(Name) {}
};

// Contains info about a nod's member. This info is used whenever a member is right-clicked and a popup shows up
struct PopupMemberInfo
{
	const std::string Name = "";
	CMwMemberInfo* MemberInfo = nullptr;
	CMwClassInfo* ParentClassInfo = nullptr;
	uintptr_t ParentNod = 0;

	// When the member is CLASS-adjacent, this value is non-null and it will be the member itself
	uintptr_t MemberNodItself = 0;

	PopupMemberInfo() = delete;
	PopupMemberInfo(PopupMemberInfo&) = delete;
	PopupMemberInfo& operator=(const PopupMemberInfo&) = delete;

	PopupMemberInfo(const std::string Name) : Name(Name) {}
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
	PopupMemberInfo* ChildInfo = nullptr;

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

	void RenderNod(uintptr_t Nod, const std::string Name, CMwMemberInfo* NodMemberInfo);
	void RenderNodInfoClassInfo(CMwClassInfo* ClassInfo);
	void RenderNodInfoMemberInfo();

	void SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const std::string NodName, uintptr_t Nod, CMwClassInfo* ClassInfo);
	void SetClassInfoPopup(CMwClassInfo* ClassInfo, const std::string NodName, uintptr_t Nod);

	std::string GetFancyMemberName(CMwMemberInfo* MemberInfo);
};