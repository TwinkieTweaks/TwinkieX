#include "pch.h"
#include "AppExplorerModule.h"
#include <format>

#define HasRightClickedOnItem() IsMouseClicked(ImGuiMouseButton_Right) and IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)

constexpr bool IsMemberClassAdjacent(CMwMemberInfo* Member)
{
	std::vector<CMwMemberInfo::eType> ClassTypes = {
		CMwMemberInfo::CLASS
#ifdef TMCN
		,
		CMwMemberInfo::CLASSALT,
		CMwMemberInfo::CLASSNOTPERSISTENT
#endif
	};

	return std::find(ClassTypes.begin(), ClassTypes.end(), Member->MemberType) != ClassTypes.end();
}

using namespace ImGui;

void AppExplorerModule::SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const std::string NodName, CMwNod* Nod, CMwClassInfo* ClassInfo)
{
	bool IsMemberVirtual = MemberInfo->MemberOffset == 0xFFFFFFFFU;
	
	// Fix memory leak incase of user clicking on another member while pop-up is still active
	if (ChildInfo)
	{
		delete ChildInfo;
		ChildInfo = nullptr;
	}

	if (NodInfo)
	{
		delete NodInfo;
		NodInfo = nullptr;
		NodInfoPopup = false;
	}

	this->MemberInfoPopup = true;
	ChildInfo = new PopupMemberInfo{NodName};
	ChildInfo->MemberInfo = MemberInfo;
	ChildInfo->ParentNod = Nod;
	ChildInfo->ParentClassInfo = ClassInfo;

	if (IsMemberClassAdjacent(MemberInfo) and !IsMemberVirtual)
	{
		ChildInfo->MemberNodItself = ReadAddr(CMwNod*, Nod + MemberInfo->MemberOffset);
	}
	else
	{
		ChildInfo->MemberNodItself = 0;
	}

	HasSetWindowPositionForPopup = false;
}

void AppExplorerModule::SetClassInfoPopup(CMwClassInfo* ClassInfo, const std::string NodName, CMwNod* Nod)
{
	// Fix memory leak incase of user clicking on another nod while pop-up is still active
	if (NodInfo)
	{
		delete NodInfo;
		NodInfo = nullptr;
	}

	if (ChildInfo)
	{
		delete ChildInfo;
		ChildInfo = nullptr;
		MemberInfoPopup = false;
	}

	NodInfoPopup = true;
	NodInfo = new PopupNodInfo{NodName};
	NodInfo->ClassInfo = ClassInfo;
	NodInfo->Nod = Nod;

	HasSetWindowPositionForPopup = false;
}

std::string AppExplorerModule::GetFancyMemberName(CMwMemberInfo* MemberInfo)
{
	std::string MemberNameFallback = MemberInfo->MemberName;

	if (MemberNameFallback == "")
	{
		MemberNameFallback = std::format("__0x{:08x}", MemberInfo->MemberID);
	}

	std::string FancyMemberName = std::format("{} (+0x{:x})",
		MemberNameFallback,
		(uint32_t)MemberInfo->MemberOffset
	);

	return FancyMemberName;
}

void AppExplorerModule::RenderNod(CMwNod* Nod, const std::string NodName, CMwMemberInfo* NodMemberInfo = nullptr)
{
	// TODO: Compare intended nod info (NodMemberInfo) with nod's actual info (ClassInfo below)
	if (!NodMemberInfo) NodMemberInfo = nullptr;

	if (!Nod)
	{
		BeginDisabled();
		Text(NodName.c_str());
		EndDisabled();

		return;
	}

	CMwClassInfo* ClassInfo = 
#ifdef TMCN
	nullptr;
	if (NodMemberInfo and NodMemberInfo->MemberType == CMwMemberInfo::CLASSNOTPERSISTENT)
	{
		ClassInfo = ((CMwMemberInfoClass*)NodMemberInfo)->ClassInfo;
	}
	else
	{
		ClassInfo =
#endif
			Nod->MwGetClassInfo();
#ifdef TMCN
	}
#endif

	std::string NodAddressStr = std::format("{:p}", (void*)Nod);
	std::string TreeNodeLabel = std::format("{}* {} ({})",
		ClassInfo->ClassName,
		NodName,
		NodAddressStr
	);

	PushID((void*)Nod);
	bool TreeNodeIsOpen = TreeNodeEx(TreeNodeLabel.c_str(), ImGuiTreeNodeFlags_DrawLinesFull);

	if (HasRightClickedOnItem())
	{
		SetClassInfoPopup(ClassInfo, NodName, Nod);
	}

	if (TreeNodeIsOpen)
	{
		while (ClassInfo)
		{
			PushID(ClassInfo);

			for (auto& MemberInfo : *ClassInfo)
			{
				std::string FancyMemberName = GetFancyMemberName(MemberInfo);

				bool IsMemberVirtual = MemberInfo->MemberOffset == 0xFFFFFFFFU;
				bool IgnoreRightClicks = false;

				switch (MemberInfo->MemberType)
				{
#ifdef TMCN
					case CMwMemberInfo::CLASSNOTPERSISTENT:
					case CMwMemberInfo::CLASSALT:
#endif
					case CMwMemberInfo::CLASS:
					{
						if (!IsMemberVirtual)
						{
							CMwNod* ChildNod = ReadAddr(CMwNod*, (uintptr_t)Nod + MemberInfo->MemberOffset);
							RenderNod(ChildNod, std::format("{} (+0x{:x})", MemberInfo->MemberName, MemberInfo->MemberOffset), MemberInfo);
						}
						else IgnoreRightClicks = true;
						break;
					}

					case CMwMemberInfo::ACTION:
					{
						const auto* MemberInfoAction = static_cast<const CMwMemberInfoAction*>(MemberInfo);
						if (Button(FancyMemberName.c_str()))
						{
							MemberInfoAction->Action(Nod);
						}
						break;
					}

					case CMwMemberInfo::CLASSBUFFER:
					{
						if (!IsMemberVirtual)
						{
							CFastBuffer<CMwNod*>* Array = reinterpret_cast<CFastBuffer<CMwNod*>*>((uintptr_t)Nod + MemberInfo->MemberOffset);
							if (Array)
							{
								PushID((void*)Array);

								CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
								CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;

								std::string ElementNameSingular =
									MemberInfoArray->ElementNameSingular ? MemberInfoArray->ElementNameSingular :
									MemberInfoArray->MemberName;
								std::string ArrayName = std::format("CFastBuffer<{}>* {}", ArrayClassType->ClassName, FancyMemberName);

								bool ArrayTreeNodeOpened = TreeNodeEx(ArrayName.c_str(), ImGuiTreeNodeFlags_DrawLinesFull);

								if (HasRightClickedOnItem())
								{
									SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo);
								}

								if (ArrayTreeNodeOpened)
								{
									for (auto& ArrayNod : *Array)
									{
										RenderNod(ArrayNod, ElementNameSingular);
									}
									TreePop();
								}
								PopID();
							}
							else
							{
								BeginDisabled();
								Text("%s CLASSBUFFER (0x%llx)", MemberInfo);
								EndDisabled();
							}
						}
						else IgnoreRightClicks = true;
						break;
					}
					case CMwMemberInfo::CLASSARRAY:
					{
						if (!IsMemberVirtual)
						{
							CFastArray<CMwNod*>* Array = reinterpret_cast<CFastArray<CMwNod*>*>((uintptr_t)Nod + MemberInfo->MemberOffset);
							if (Array)
							{
								PushID((void*)Array);

								CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
								CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;

								std::string ElementNameSingular =
									MemberInfoArray->ElementNameSingular ? MemberInfoArray->ElementNameSingular :
									MemberInfoArray->MemberName;
								std::string ArrayName = std::format("CFastBuffer<{}>* {}", ArrayClassType->ClassName, FancyMemberName);

								bool ArrayTreeNodeOpened = TreeNodeEx(ArrayName.c_str(), ImGuiTreeNodeFlags_DrawLinesFull);

								if (HasRightClickedOnItem())
								{
									SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo);
								}

								if (ArrayTreeNodeOpened)
								{
									for (auto& ArrayNod : *Array)
									{
										RenderNod(ArrayNod, ElementNameSingular);
									}
									TreePop();
								}
								PopID();
							}
							else
							{
								BeginDisabled();
								Text("%s CLASSARRAY (0x%llx)", MemberInfo);
								EndDisabled();
							}
						}
						else IgnoreRightClicks = true;
						break;
					}

					case CMwMemberInfo::REAL:
					{
						if (!IsMemberVirtual)
						{
							float* Float = (float*)((uintptr_t)Nod + MemberInfo->MemberOffset);
							InputFloat(FancyMemberName.c_str(), Float);
						}
						else IgnoreRightClicks = true;
						break;
					}
#ifdef TMCN
					case CMwMemberInfo::REALRANGE:
					{
						if (!IsMemberVirtual)
						{
							CMwMemberInfoRealRange* MemberAsRealRange = (CMwMemberInfoRealRange*)MemberInfo;
							float* Float = (float*)((uintptr_t)Nod + MemberInfo->MemberOffset);
							SliderFloat(FancyMemberName.c_str(), Float, MemberAsRealRange->ValueMin, MemberAsRealRange->ValueMax);
						}
						else IgnoreRightClicks = true;
						break;
					}

					case CMwMemberInfo::INTRANGE:
					{
						if (!IsMemberVirtual)
						{
							CMwMemberInfoIntRange* MemberAsIntRange = (CMwMemberInfoIntRange*)MemberInfo;
							int* Int = (int*)((uintptr_t)Nod + MemberInfo->MemberOffset);
							SliderInt(FancyMemberName.c_str(), Int, MemberAsIntRange->ValueMin, MemberAsIntRange->ValueMax);
						}
						else IgnoreRightClicks = true;
						break;
					}

					case CMwMemberInfo::ENUM:
					{
						if (!IsMemberVirtual)
						{
							CMwMemberInfoEnum* MemberAsEnum = (CMwMemberInfoEnum*)MemberInfo;
							int* CurrentItem = (int*)((uintptr_t)Nod + MemberInfo->MemberOffset);
							Combo((MemberAsEnum->EnumTypeName + (" " + FancyMemberName)).c_str(), CurrentItem, MemberAsEnum->EnumValueNames, MemberAsEnum->EnumValueNamesLength);
						}
						else IgnoreRightClicks = true;
						break;
					}
#endif

					case CMwMemberInfo::BOOL:
					{
						if (!IsMemberVirtual)
						{
							bool* Bool = (bool*)((uintptr_t)Nod + MemberInfo->MemberOffset);
							Checkbox(FancyMemberName.c_str(), Bool);
						}
						else IgnoreRightClicks = true;
						break;
					}

					default:
					{
						Text("%s (%s)", FancyMemberName.c_str(), g_MemberTypeNames[MemberInfo->MemberType]);
						break;
					}
				}
				if (HasRightClickedOnItem() and not IgnoreRightClicks)
				{
					SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo);
				}
			}
			
			if (ClassInfo->ParentClassInfo)
			{
				SeparatorText(ClassInfo->ParentClassInfo->ClassName);

				if (HasRightClickedOnItem())
				{
					SetClassInfoPopup(ClassInfo->ParentClassInfo, NodName, Nod);
				}
			}

			// This is to fix a nullptr exception when trying to access the classinfo for popup because we break by default when classinfo is null
			// Technically, this makes the "while (ClassInfo)" clause above useless (i.e. could be infinite loop), but I'd rather not :P
			if (!ClassInfo->ParentClassInfo)
			{
				PopID();
				break;
			}
			ClassInfo = ClassInfo->ParentClassInfo;

			PopID();
		}
		TreePop();
	}

	PopID();
}

void AppExplorerModule::RenderMenu()
{
	if (MenuItem("AppExplorer"))
	{
		Visible = !Visible;
	}
}

void AppExplorerModule::RenderNodInfoClassInfo(CMwClassInfo* ClassInfo)
{
	{
		std::string Copyable = ClassInfo->ClassName;
		if (Selectable((std::string("Copy: ") + Copyable).c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		std::string Copyable = std::vformat("{:x}", std::make_format_args(ClassInfo->ClassID));
		if (Selectable((std::string("Copy: ") + Copyable).c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		void* ClassInfoAddr = reinterpret_cast<void*>(ClassInfo);
		std::string Copyable = std::format("{:p}", ClassInfoAddr);
		if (Selectable((std::string("Copy: ") + Copyable + " (class info ptr)").c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
}

void AppExplorerModule::RenderNodInfoMemberInfo()
{
	CMwMemberInfo* MemberInfo = ChildInfo->MemberInfo;
	{
		std::string Copyable = MemberInfo->MemberName;
		if (Selectable((std::string("Copy: ") + Copyable).c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		std::string Copyable = std::vformat("{:x}", std::make_format_args(MemberInfo->MemberID));
		if (Selectable((std::string("Copy: ") + Copyable).c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		void* MemberInfoAddr = reinterpret_cast<void*>(MemberInfo);
		std::string Copyable = std::format("{:p}", MemberInfoAddr);
		if (Selectable((std::string("Copy: ") + Copyable + " (member info ptr)").c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	if (ChildInfo->MemberNodItself)
	{
		Separator();

		CMwClassInfo* MemberNodClassInfo =
#ifdef TMCN
			nullptr;
		if (ChildInfo->MemberInfo->MemberType == CMwMemberInfo::CLASSNOTPERSISTENT)
		{
			MemberNodClassInfo = ((CMwMemberInfoClass*)ChildInfo->MemberInfo)->ClassInfo;
		}
		else
		{
			MemberNodClassInfo = 
#endif
				ChildInfo->MemberNodItself->MwGetClassInfo();
#ifdef TMCN
		}
#endif
		RenderNodInfoClassInfo(MemberNodClassInfo);
	}
}

void AppExplorerModule::RenderInterface()
{
	if (!Visible) return;

	Begin("AppExplorer");

	RenderNod(Twinkie->GetApp(), "App");

	End();

	if (NodInfoPopup)
	{
		Begin("##AppExplorerNodInfoPopup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

		if (!IsWindowFocused())
		{
			NodInfoPopup = false;
			if (NodInfo)
			{
				delete NodInfo;
				NodInfo = nullptr;

				End();
				goto ContinueAfterRender;
			}
		}
		else if (!HasSetWindowPositionForPopup)
		{
			HasSetWindowPositionForPopup = true;
			SetWindowPos(GetMousePos());
		}
		
		{
			std::string Copyable = NodInfo->Name;
			if (Selectable((std::string("Copy: ") + Copyable).c_str()))
			{
				SetClipboardText(Copyable.c_str());
				NodInfoPopup = false;
			}
		}
		{
			std::string Copyable = std::format("{:p}", (void*)NodInfo->Nod);
			if (Selectable((std::string("Copy: ") + Copyable + " (nod ptr)").c_str()))
			{
				SetClipboardText(Copyable.c_str());
				NodInfoPopup = false;
			}
		}
		
		RenderNodInfoClassInfo(NodInfo->ClassInfo);

		End();
	}
	else if (MemberInfoPopup)
	{
		Begin("##AppExplorerMemberInfoPopup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

		if (!IsWindowFocused())
		{
			MemberInfoPopup = false;
			if (ChildInfo)
			{
				delete ChildInfo;
				ChildInfo = nullptr;

				End();
				goto ContinueAfterRender;
			}
		}
		else if (!HasSetWindowPositionForPopup)
		{
			HasSetWindowPositionForPopup = true;
			SetWindowPos(GetMousePos());
		}

		RenderNodInfoMemberInfo();

		End();
	}

ContinueAfterRender:
	if (HasSetWindowPositionForPopup and (!NodInfoPopup and !MemberInfoPopup))
	{
		HasSetWindowPositionForPopup = false;
	}
	return;
}