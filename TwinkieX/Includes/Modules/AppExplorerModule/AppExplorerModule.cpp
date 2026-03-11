#include "pch.h"
#include "AppExplorerModule.h"
#include <string>
#include <format>

using namespace ImGui;

void AppExplorerModule::RenderNod(uintptr_t Nod, const char* const NodName, CMwMemberInfo* NodMemberInfo = nullptr)
{
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
	std::string TreeNodeLabel = std::format("{}* {} (0x{})",
		ClassInfo->ClassName,
		NodName,
		NodAddressStr
	);

	PushID((void*)Nod);
	if (TreeNode(TreeNodeLabel.c_str()))
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
								if (TreeNode(MemberInfo->MemberName))
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
								if (TreeNode(MemberInfo->MemberName))
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
			}
			
			if (ClassInfo->ParentClassInfo)
			{
				SeparatorText(ClassInfo->ParentClassInfo->ClassName);
			}
			ClassInfo = ClassInfo->ParentClassInfo;

			PopID();
		}
		TreePop();
	}

	if (IsMouseClicked(ImGuiMouseButton_Right) and IsItemHovered())
	{
		NodInfoPopup = true;
		NodInfo = new PopupNodInfo;
		NodInfo->ClassInfo = ClassInfo;
		NodInfo->Name = NodName;
		NodInfo->Nod = Nod;
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

void AppExplorerModule::RenderInterface()
{
	static bool HasSetWindowPosition = false;

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
		else if (!HasSetWindowPosition)
		{
			HasSetWindowPosition = true;
			SetWindowPos(GetMousePos());
		}

		{
			std::string Copyable = NodInfo->ClassInfo->ClassName;
			if (Selectable((std::string("Copy: ") + Copyable).c_str()))
			{
				SetClipboardText(Copyable.c_str());
				NodInfoPopup = false;
			}
		}
		{
			std::string Copyable = std::vformat("{:x}", std::make_format_args(NodInfo->ClassInfo->ClassID));
			if (Selectable((std::string("Copy: ") + Copyable).c_str()))
			{
				SetClipboardText(Copyable.c_str());
				NodInfoPopup = false;
			}
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
			void* ClassInfoAddr = reinterpret_cast<void*>(NodInfo->ClassInfo);
			std::string Copyable = std::format("{:p}", ClassInfoAddr);
			if (Selectable((std::string("Copy: ") + Copyable + " (class info ptr)").c_str()))
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

		End();
	}

ContinueAfterRender:
	if (HasSetWindowPosition and !NodInfoPopup)
	{
		HasSetWindowPosition = false;
	}
	return;
}