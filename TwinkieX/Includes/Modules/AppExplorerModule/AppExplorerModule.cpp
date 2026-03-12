#include "pch.h"
#include "AppExplorerModule.h"
#include <string>
#include <format>

#define HasRightClickedOnItem() IsMouseClicked(ImGuiMouseButton_Right) and IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)

using namespace ImGui;

void AppExplorerModule::SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const char* NodName, uintptr_t Nod, CMwClassInfo* ClassInfo)
{
	bool IsMemberVirtual = MemberInfo->MemberOffset == 0xFFFFFFFFU;
	
	// Fix memory leak incase of user clicking on another member while pop-up is still active
	if (this->MemberInfo)
	{
		delete this->MemberInfo;
		this->MemberInfo = nullptr;
	}

	if (NodInfo)
	{
		delete NodInfo;
		NodInfo = nullptr;
		NodInfoPopup = false;
	}

	this->MemberInfoPopup = true;
	this->MemberInfo = new PopupMemberInfo;
	this->MemberInfo->MemberInfo = MemberInfo;
	this->MemberInfo->Name = NodName;
	this->MemberInfo->ParentNod = Nod;
	this->MemberInfo->ParentClassInfo = ClassInfo;

	// TODO: Fix for other CLASS-adjacent types and for virtual members
	if (MemberInfo->MemberType == CMwMemberInfo::CLASS and !IsMemberVirtual)
	{
		this->MemberInfo->MemberNodItself = ReadAddr(uintptr_t, Nod + MemberInfo->MemberOffset);
	}
	else
	{
		this->MemberInfo->MemberNodItself = 0;
	}

	HasSetWindowPositionForPopup = false;
}

void AppExplorerModule::SetClassInfoPopup(CMwClassInfo* ClassInfo, const char* NodName, uintptr_t Nod)
{
	// Fix memory leak incase of user clicking on another nod while pop-up is still active
	if (NodInfo)
	{
		delete NodInfo;
		NodInfo = nullptr;
	}

	if (MemberInfo)
	{
		delete MemberInfo;
		MemberInfo = nullptr;
		MemberInfoPopup = false;
	}

	NodInfoPopup = true;
	NodInfo = new PopupNodInfo;
	NodInfo->ClassInfo = ClassInfo;
	NodInfo->Name = NodName;
	NodInfo->Nod = Nod;

	HasSetWindowPositionForPopup = false;
}

void AppExplorerModule::RenderNod(uintptr_t Nod, const char* const NodName, CMwMemberInfo* NodMemberInfo = nullptr)
{
	bool IsMemberOpen = false;

	// TODO: Compare intended nod info (NodMemberInfo) with nod's actual info (ClassInfo below)
	if (!NodMemberInfo) NodMemberInfo = nullptr;

	if (Nod == 0)
	{
		BeginDisabled();
		Text(NodName);
		EndDisabled();

		return;
	}

	auto ClassInfo = Twinkie->GetNodClassInfo(Nod);

	std::string NodAddressStr = std::format("{:p}", (void*)Nod);
	std::string TreeNodeLabel = std::format("{}* {} ({})",
		ClassInfo->ClassName,
		NodName,
		NodAddressStr
	);

	PushID((void*)Nod);
	bool TreeNodeIsOpen = TreeNode(TreeNodeLabel.c_str());

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
				bool IsMemberVirtual = MemberInfo->MemberOffset == 0xFFFFFFFFU;

				switch (MemberInfo->MemberType)
				{
					case CMwMemberInfo::CLASS:
					{
						if (!IsMemberVirtual)
						{
							uintptr_t ChildNod = ReadAddr(uintptr_t, Nod + MemberInfo->MemberOffset);
							RenderNod(ChildNod, MemberInfo->MemberName, MemberInfo);
						}
						break;
					}

					case CMwMemberInfo::ACTION:
					{
						const auto* MemberInfoAction = static_cast<const CMwMemberInfoAction*>(MemberInfo);
						if (Button(MemberInfo->MemberName))
						{
							MemberInfoAction->Action(Nod);
						}
						break;
					}

					case CMwMemberInfo::CLASSBUFFER:
					{
						if (!IsMemberVirtual)
						{
							CFastBuffer<uintptr_t>* Array = reinterpret_cast<CFastBuffer<uintptr_t>*>(Nod + MemberInfo->MemberOffset);
							if (Array)
							{
								PushID((void*)Array);
								bool ArrayTreeNodeOpened = TreeNode(MemberInfo->MemberName);

								if (HasRightClickedOnItem())
								{
									SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo);
								}

								if (ArrayTreeNodeOpened)
								{
									for (auto& ArrayNod : *Array)
									{
										RenderNod(ArrayNod, MemberInfo->MemberName);
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
						break;
					}
					case CMwMemberInfo::CLASSARRAY:
					{
						if (!IsMemberVirtual)
						{
							CFastArray<uintptr_t>* Array = reinterpret_cast<CFastArray<uintptr_t>*>(Nod + MemberInfo->MemberOffset);
							if (Array)
							{
								PushID((void*)Array);
								bool ArrayTreeNodOpened = TreeNode(MemberInfo->MemberName);

								if (HasRightClickedOnItem())
								{
									SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo);
								}

								if (ArrayTreeNodOpened)
								{
									for (auto& ArrayNod : *Array)
									{
										RenderNod(ArrayNod, MemberInfo->MemberName);
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
						break;
					}

					case CMwMemberInfo::REAL:
					{
						if (!IsMemberVirtual)
						{
							float* Float = (float*)(Nod + MemberInfo->MemberOffset);
							InputFloat(MemberInfo->MemberName, Float);
						}
						break;
					}

					case CMwMemberInfo::BOOL:
					{
						if (!IsMemberVirtual)
						{
							bool* Bool = (bool*)(Nod + MemberInfo->MemberOffset);
							Checkbox(MemberInfo->MemberName, Bool);
						}
						break;
					}

					default:
					{
						Text("%s (+0x%x) (%d)", MemberInfo->MemberName, MemberInfo->MemberOffset, MemberInfo->MemberType);
						break;
					}
				}
				if (HasRightClickedOnItem())
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
	CMwMemberInfo* MemberInfo = this->MemberInfo->MemberInfo;
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
	if (this->MemberInfo->MemberNodItself)
	{
		Separator();

		RenderNodInfoClassInfo(Twinkie->GetNodClassInfo(this->MemberInfo->MemberNodItself));
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

		Separator();
		
		RenderNodInfoClassInfo(NodInfo->ClassInfo);

		End();
	}
	else if (MemberInfoPopup)
	{
		Begin("##AppExplorerMemberInfoPopup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

		if (!IsWindowFocused())
		{
			MemberInfoPopup = false;
			if (MemberInfo)
			{
				delete MemberInfo;
				MemberInfo = nullptr;

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
			std::string Copyable = MemberInfo->Name;
			if (Selectable((std::string("Copy: ") + Copyable).c_str()))
			{
				SetClipboardText(Copyable.c_str());
				MemberInfoPopup = false;
			}
		}
		{
			std::string Copyable = std::format("{:p}", (void*)MemberInfo->ParentNod);
			if (Selectable((std::string("Copy: ") + Copyable + " (nod ptr)").c_str()))
			{
				SetClipboardText(Copyable.c_str());
				MemberInfoPopup = false;
			}
		}

		Separator();

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