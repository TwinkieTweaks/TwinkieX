#pragma once
#include <Modules/IModule.h>
#include <string>
#include <map>

constexpr bool IsMemberClassAdjacent(CMwMemberInfo* Member);
namespace Gecko::Exports::Trackmania 
{
	extern std::unordered_map<uint32_t, CMwClassInfo*> ClassIDToInfo;

#ifdef TMCN
	extern std::unordered_map<uint32_t, CMwEnumInfo*> EnumIDToInfo;
	extern std::unordered_map<std::string, CMwEnumInfo*> EnumNameToInfo;
#endif
}

// Contains info about a nod. This info is used whenever a nod is right-clicked and a popup shows up
struct PopupNodInfo
{
	const std::string Name = "";
	CMwClassInfo* ClassInfo = nullptr;
	CMwNod* Nod = 0;

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
	void* MemberPtr = nullptr;
	CMwNod* ParentNod = 0;

	// When the member is CLASS-adjacent, this value is non-null and it will be the member itself
	CMwNod* MemberNodItself = 0;

	PopupMemberInfo() = delete;
	PopupMemberInfo(PopupMemberInfo&) = delete;
	PopupMemberInfo& operator=(const PopupMemberInfo&) = delete;

	PopupMemberInfo(const std::string Name) : Name(Name) {}
};

// Inputs and outputs of a PROC called from the explorer
struct ProcCallState
{
	struct ParamValue
	{
		// The value passed to the PROC. Big enough for any parameter type
		alignas(16) uint8_t Storage[64] = {};
		// Text input for strings and nod addresses
		char Text[256] = {};
		// Keep input strings alive during the call
		std::string NarrowText;
		std::wstring WideText;
	};

	std::vector<ParamValue> Values;
	bool HasResult = false;
	uint32_t Result = 0;
};

// std::pair has no std::hash
struct ProcCallKeyHash
{
	size_t operator()(const std::pair<CMwNod*, CMwMemberInfo*>& Key) const
	{
		return std::hash<CMwNod*>{}(Key.first) ^ (std::hash<CMwMemberInfo*>{}(Key.second) << 1);
	}
};

class NodExplorerModule : public IModule
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

	// PROC call inputs and results, per nod and PROC
	std::unordered_map<std::pair<CMwNod*, CMwMemberInfo*>, ProcCallState, ProcCallKeyHash> ProcCalls;

public:
	NodExplorerModule(TwinkTrackmania& Twinkie, ILoggert& Loggert) : IModule{ Twinkie, Loggert, "NodExplorerModule", "Nod Explorer" } {};

	virtual ~NodExplorerModule() = default;

	virtual void RenderInterface() override;
	virtual void RenderMenu() override;

	void RenderNod(CMwNod* Nod, const std::string Name, CMwMemberInfo* NodMemberInfo = nullptr, CMwClassInfo* StaticClassInfo = nullptr);
	bool RenderArrayMember(CMwNod* Nod, CMwMemberInfo* MemberInfo, const std::string& FancyMemberName, const std::string& NodName, CMwClassInfo* ClassInfo, bool IsNodImpersistent, bool& IgnoreRightClicks);
	void RenderNodInfoClassInfo(CMwClassInfo* ClassInfo);
	void RenderNodInfoMemberInfo();
	void RenderProcCall(CMwNod* Nod, CMwMemberInfoProc* Proc, const std::vector<CMwMemberInfoProc::Param>& Params);

	void SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const std::string NodName, CMwNod* Nod, CMwClassInfo* ClassInfo, bool IsNodImpersistent = false);
	void SetClassInfoPopup(CMwClassInfo* ClassInfo, const std::string NodName, CMwNod* Nod, bool IsNodImpersistent = false);

	std::string GetFancyMemberName(CMwMemberInfo* MemberInfo);
};