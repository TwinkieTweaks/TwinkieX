#include "pch.h"
#include "NodExplorerModule.h"
#include <Modules/ModuleAutoExport.h>
#include <Twinkie/TwinkUi/TwinkUi.h>
#include <format>

#define HasRightClickedOnItem() IsMouseClicked(ImGuiMouseButton_Right) and IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)

using namespace Gecko::Exports::Trackmania;

constexpr bool IsMemberClassAdjacent(CMwMemberInfo* Member)
{
	std::vector<CMwMemberInfo::EType> ClassTypes = {
		CMwMemberInfo::CLASS
#ifdef TMCN
		,
		CMwMemberInfo::NODREF,
		CMwMemberInfo::CLASSNOTPERSISTENT
#endif
	};

	return std::find(ClassTypes.begin(), ClassTypes.end(), Member->MemberType) != ClassTypes.end();
}

using namespace ImGui;

void NodExplorerModule::SetMemberInfoPopup(CMwMemberInfo* MemberInfo, const std::string NodName, CMwNod* Nod, CMwClassInfo* ClassInfo, bool IsNodImpersistent)
{
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
	ChildInfo->MemberPtr = Twinkie->ParamGet<uintptr_t /* literally anything works */>(Nod, MemberInfo, IsNodImpersistent);

	if (IsMemberClassAdjacent(MemberInfo))
	{
		ChildInfo->MemberNodItself = Twinkie->ParamGet<CMwNod*>(Nod, MemberInfo, IsNodImpersistent);
	}

	HasSetWindowPositionForPopup = false;
}

void NodExplorerModule::SetClassInfoPopup(CMwClassInfo* ClassInfo, const std::string NodName, CMwNod* Nod, [[maybe_unused]] bool IsNodImpersistent)
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

#ifndef TM1
static std::string WideToUtf8(const wchar_t* Str, uint32_t Count)
{
	if (!Str || !Count) return "";
	int Size = WideCharToMultiByte(CP_UTF8, 0, Str, (int)Count, nullptr, 0, nullptr, nullptr);
	if (Size <= 0) return "";
	std::string Utf8((size_t)Size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, Str, (int)Count, Utf8.data(), Size, nullptr, nullptr);
	return Utf8;
}
#endif

static std::string ReadGameString(const CFastString& String)
{
	if (!String.CStr) return "";
#ifdef TM1
	return std::string(String.CStr, strnlen(String.CStr, String.Count));
#else
	return std::string(String.CStr, String.Count);
#endif
}

static void RenderStringValue(const char* Label, std::string Shown)
{
	InputText(Label, Shown.data(), Shown.size() + 1, ImGuiInputTextFlags_ReadOnly);
}

std::string NodExplorerModule::GetFancyMemberName(CMwMemberInfo* MemberInfo)
{
	std::string MemberNameFallback = MemberInfo->MemberName;

	if (MemberNameFallback.empty())
	{
		MemberNameFallback = std::format("__0x{:08x}", MemberInfo->MemberID);
	}

	std::string FancyMemberName = std::format("{} (+0x{:x})",
		MemberNameFallback,
		(uint32_t)MemberInfo->MemberOffset
	);

	return FancyMemberName;
}

void NodExplorerModule::RenderNod(CMwNod* Nod, const std::string NodName, CMwMemberInfo* NodMemberInfo = nullptr)
{
	bool IsNodVirtual = false;
	if (NodMemberInfo) IsNodVirtual = NodMemberInfo->MemberOffset == 0xFFFFFFFFU;

	bool IsNodImpersistent = false;
#ifdef TMCN
	if (NodMemberInfo) IsNodImpersistent = NodMemberInfo->MemberType == CMwMemberInfo::CLASSNOTPERSISTENT;
#endif

	CMwMemberInfoClass* MemberInfoAsClass = (CMwMemberInfoClass*)NodMemberInfo;
	if (!Nod)
	{
		BeginDisabled();
		if (NodMemberInfo)
		{
			Text("%s* %s", MemberInfoAsClass->ClassInfo->ClassName, NodName.c_str());
		}
		else
		{
			Text(NodName.c_str());
		}
		EndDisabled();
		return;
	}

	CMwClassInfo* ClassInfo = IsNodImpersistent ? MemberInfoAsClass->ClassInfo : Nod->MwGetClassInfo();

	std::string NodAddressStr = std::format("{:p}", (void*)Nod);
	std::string ClassTypeName = std::format("{}*{}",
		ClassInfo->ClassName,
		MemberInfoAsClass and (MemberInfoAsClass->ClassInfo->ClassID != ClassInfo->ClassID) ? std::string(" \\$999(") + MemberInfoAsClass->ClassInfo->ClassName + "*)\\$z" : std::string("")
	);
	std::string TreeNodeLabel = std::format("{}{} {} ({})",
		IsNodVirtual ? std::string("\\$66fV\\$z ") : std::string(""),
		ClassTypeName,
		NodName,
		NodAddressStr
	);

	// TODO: Is this *really* needed?
	// PushID((void*)Nod);
	// Alternative:
	PushID((void*)NodMemberInfo);

	bool TreeNodeIsOpen = TreeNodeEx(TreeNodeLabel.c_str(), ImGuiTreeNodeFlags_DrawLinesFull);

	if (HasRightClickedOnItem())
	{
		SetClassInfoPopup(ClassInfo, NodName, Nod, IsNodImpersistent);
	}

	if (TreeNodeIsOpen)
	{
		while (ClassInfo)
		{
			PushID(ClassInfo);

			for (auto& MemberInfo : *ClassInfo)
			{
				std::string FancyMemberName = GetFancyMemberName(MemberInfo);

				bool IgnoreRightClicks = false;

				switch (MemberInfo->MemberType)
				{
				case CMwMemberInfo::PROC:
				{
					auto ParamTypeToStr = [](const CMwMemberInfoProc::Type& Type) -> std::string {
						switch (Type.Kind)
						{
						case CMwMemberInfoProc::Type::VALUE:
							return Type.AdditionalInfo < CMwMemberInfo::MAX ? g_MemberTypeSignatures[Type.AdditionalInfo] : std::to_string(Type.AdditionalInfo);
						case CMwMemberInfoProc::Type::CLASS:
							return ClassIDToInfo.contains(Type.AdditionalInfo) ? std::string(ClassIDToInfo[Type.AdditionalInfo]->ClassName) + "@" : std::format("0x{:x}@", Type.AdditionalInfo);
						case CMwMemberInfoProc::Type::ENUM:
							return 
#ifdef TMCN
								EnumIDToInfo.contains(Type.AdditionalInfo) ? std::string(EnumIDToInfo[Type.AdditionalInfo]->EnumName) : 
#endif
								std::format("enum<0x{:x}", Type.AdditionalInfo);
						}
						return std::format("?<{}>", (uint32_t)Type.Kind);
						};

					auto ParamToStr = [&ParamTypeToStr](const CMwMemberInfoProc::Param& Param) {
						return std::format(
#ifdef _DEBUG
							"{} {} ({}, {:x})",
							ParamTypeToStr(Param.Type),
							Param.Name,
							(uint32_t)Param.Type.Kind,
							(uint32_t)Param.Flags
#else
							"{} {}",
							ParamTypeToStr(Param.Type),
							Param.Name
#endif
						);
						};

					const CMwMemberInfoProc::Param* ReturnParam = nullptr;

					auto MemberInfoAsProc = (CMwMemberInfoProc*)MemberInfo;

					std::vector<CMwMemberInfoProc::Param> Params = MemberInfoAsProc->GetParams();

					std::string ActualProcParameters = "";

					for (auto& Param : Params)
					{
						if ((Param.Flags & CMwMemberInfoProc::PARAM_OUT) and not ReturnParam) ReturnParam = &Param;
						else
						{
							ActualProcParameters += ParamToStr(Param) + ", ";
						}
					}

					std::string CumulativeProcSignature = ReturnParam ? ParamTypeToStr(ReturnParam->Type) + " " : "void ";
					CumulativeProcSignature += MemberInfo->MemberName;
					CumulativeProcSignature += " (";
					CumulativeProcSignature += ActualProcParameters;
					// Remove extra ", "
					CumulativeProcSignature = CumulativeProcSignature.substr(0, CumulativeProcSignature.length() - min(ActualProcParameters.length(), 2));
					CumulativeProcSignature += ")";

					PushID((void*)MemberInfo);
					bool ProcTreeNodeOpened = TreeNodeEx(("PROC " + CumulativeProcSignature).c_str(), ImGuiTreeNodeFlags_DrawLinesFull);

					if (HasRightClickedOnItem())
					{
						SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo, IsNodImpersistent);
					}
					IgnoreRightClicks = true;

					if (ProcTreeNodeOpened)
					{
						RenderProcCall(Nod, MemberInfoAsProc, Params);
						TreePop();
					}
					PopID();

					break;
				}
#ifdef TMCN
				case CMwMemberInfo::CLASSNOTPERSISTENT:
				case CMwMemberInfo::NODREF:
#endif
				case CMwMemberInfo::CLASS:
				{
					CMwNod* ChildNod = Twinkie->ParamGet<CMwNod*>(Nod, MemberInfo, IsNodImpersistent);

					RenderNod(ChildNod, std::format("{} (+0x{:x})", MemberInfo->MemberName, MemberInfo->MemberOffset), MemberInfo);

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
					CFastBuffer<CMwNod*>* Array = Twinkie->ParamGet<CFastBuffer<CMwNod*>>(Nod, MemberInfo, IsNodImpersistent);
					if (Array)
					{
						PushID((void*)Array);

						CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
						CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;

						std::string ElementNameSingular =
#ifndef TMU
							MemberInfoArray->ElementNameSingular ? MemberInfoArray->ElementNameSingular :
#endif
							MemberInfoArray->MemberName;
						std::string ArrayName = std::format("CFastBuffer<{}*> {}", ArrayClassType->ClassName, FancyMemberName);

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

						CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
						CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;
						std::string ArrayName = std::format("CFastBuffer<{}*> {}", ArrayClassType->ClassName, FancyMemberName);

						Text(ArrayName.c_str());

						EndDisabled();
					}
					break;
				}
				case CMwMemberInfo::CLASSARRAY:
				{
					CFastArray<CMwNod*>* Array = Twinkie->ParamGet<CFastArray<CMwNod*>>(Nod, MemberInfo, IsNodImpersistent);
					if (Array)
					{
						PushID((void*)Array);

						CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
						CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;

						std::string ElementNameSingular =
#ifndef TMU
							MemberInfoArray->ElementNameSingular ? MemberInfoArray->ElementNameSingular :
#endif
							MemberInfoArray->MemberName;
						std::string ArrayName = std::format("CFastArray<{}*> {}", ArrayClassType->ClassName, FancyMemberName);

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

						CMwMemberInfoClassArray* MemberInfoArray = (CMwMemberInfoClassArray*)MemberInfo;
						CMwClassInfo* ArrayClassType = MemberInfoArray->ArrayClassInfo;
						std::string ArrayName = std::format("CFastArray<{}*> {}", ArrayClassType->ClassName, FancyMemberName);

						Text(ArrayName.c_str());

						EndDisabled();
					}
					break;
				}
#ifndef TMCN
				case CMwMemberInfo::REALRANGE:
#endif
#ifdef TMU
					// TODO: oh god
				case CMwMemberInfo::IDBUFFERCAT:
#endif
				case CMwMemberInfo::REAL:
				{
					float* Float = Twinkie->ParamGet<float>(Nod, MemberInfo, IsNodImpersistent);
					if (!Float) goto RerenderMember;
					float FloatV = *Float;
					if (InputFloat(FancyMemberName.c_str(), &FloatV))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &FloatV);
					}
					break;
				}
#ifdef TMCN
				case CMwMemberInfo::REALRANGE:
				{
					CMwMemberInfoRealRange* MemberAsRealRange = (CMwMemberInfoRealRange*)MemberInfo;
					float* Float = Twinkie->ParamGet<float>(Nod, MemberInfo, IsNodImpersistent);
					if (!Float) goto RerenderMember;
					float FloatV = *Float;
					if (SliderFloat(FancyMemberName.c_str(), &FloatV, MemberAsRealRange->ValueMin, MemberAsRealRange->ValueMax))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &FloatV);
					}
					break;
				}

				case CMwMemberInfo::INTRANGE:
				{
					CMwMemberInfoIntRange* MemberAsIntRange = (CMwMemberInfoIntRange*)MemberInfo;
					int* Int = Twinkie->ParamGet<int>(Nod, MemberInfo, IsNodImpersistent);
					if (!Int) goto RerenderMember;
					int IntV = *Int;
					if (SliderInt(FancyMemberName.c_str(), &IntV, MemberAsIntRange->ValueMin, MemberAsIntRange->ValueMax))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &IntV);
					}
					break;
				}

				case CMwMemberInfo::ENUM:
				{
					CMwMemberInfoEnum* MemberAsEnum = (CMwMemberInfoEnum*)MemberInfo;
					int* CurrentItem = Twinkie->ParamGet<int>(Nod, MemberInfo, IsNodImpersistent);
					if (!CurrentItem) goto RerenderMember;
					int CurrentItemV = *CurrentItem;
					if (MemberAsEnum->EnumValueNames)
					{
						if (Combo((MemberAsEnum->EnumTypeName + (" " + FancyMemberName)).c_str(), &CurrentItemV, MemberAsEnum->EnumValueNames, MemberAsEnum->EnumValueNamesLength))
						{
							Twinkie->ParamSet(Nod, MemberInfo, &CurrentItemV);
						}
					}
					else if (MemberAsEnum->EnumTypeName)
					{
						if (InputInt((MemberAsEnum->EnumTypeName + (" " + FancyMemberName)).c_str(), &CurrentItemV))
						{
							Twinkie->ParamSet(Nod, MemberInfo, &CurrentItemV);
						}
					}
					else
					{
						if (InputInt(FancyMemberName.c_str(), &CurrentItemV))
						{
							Twinkie->ParamSet(Nod, MemberInfo, &CurrentItemV);
						}
					}
					break;
				}
#endif

				case CMwMemberInfo::NATURAL:
				case CMwMemberInfo::INT:
#ifdef GAMEBOX
				case CMwMemberInfo::ENUM:
#endif
				{
					int* Int = Twinkie->ParamGet<int>(Nod, MemberInfo, IsNodImpersistent);
					if (!Int) goto RerenderMember;
					int IntV = *Int;
					if (InputInt((FancyMemberName + std::format(" {:x}", (uint64_t)(uintptr_t)(void*)Int)).c_str(), &IntV))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &IntV);
					}
					break;
				}

				case CMwMemberInfo::STRING:
				{
					CFastString* String = Twinkie->ParamGet<CFastString>(Nod, MemberInfo, IsNodImpersistent);
					if (!String) goto RerenderMember;
					RenderStringValue(FancyMemberName.c_str(), ReadGameString(*String));
					break;
				}

#ifndef TM1
				case CMwMemberInfo::STRINGINT:
				{
					CFastStringInt* String = Twinkie->ParamGet<CFastStringInt>(Nod, MemberInfo, IsNodImpersistent);
					if (!String) goto RerenderMember;
					RenderStringValue(FancyMemberName.c_str(), WideToUtf8(String->CStr, String->Count));
					break;
				}
#endif

#if defined(TMCN) || defined(TMU)
				case CMwMemberInfo::ID:
				{
					uint32_t* Id = Twinkie->ParamGet<uint32_t>(Nod, MemberInfo, IsNodImpersistent);
					if (!Id) goto RerenderMember;
					uint32_t IdV = *Id;
					if (InputScalar(FancyMemberName.c_str(), ImGuiDataType_U32, &IdV, nullptr, nullptr, "0x%08X"))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &IdV);
					}
					SameLine();
					TextDisabled("%s", Twinkie->GetMwIdName(IdV).c_str());
					break;
				}
#endif

				case CMwMemberInfo::BOOL:
				{
					bool* Bool = Twinkie->ParamGet<bool>(Nod, MemberInfo, IsNodImpersistent);
					if (!Bool) goto RerenderMember;
					bool BoolV = *Bool;
					if (Checkbox(FancyMemberName.c_str(), &BoolV))
					{
						Twinkie->ParamSet(Nod, MemberInfo, &BoolV);
					}
					break;
				}

				default:
				{
				RerenderMember:
					Text("%s (%s %d)", FancyMemberName.c_str(), g_MemberTypeNames[MemberInfo->MemberType], MemberInfo->MemberType);
					break;
				}
				}
				if (HasRightClickedOnItem() and not IgnoreRightClicks)
				{
					SetMemberInfoPopup(MemberInfo, NodName, Nod, ClassInfo, IsNodImpersistent);
				}
			}

			if (ClassInfo->ParentClassInfo)
			{
				SeparatorText(ClassInfo->ParentClassInfo->ClassName);

				if (HasRightClickedOnItem())
				{
					SetClassInfoPopup(ClassInfo->ParentClassInfo, NodName, Nod, IsNodImpersistent);
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

#ifdef TMCN
static std::wstring Utf8ToWide(const char* Str)
{
	int Size = MultiByteToWideChar(CP_UTF8, 0, Str, -1, nullptr, 0);
	if (Size <= 1) return L"";
	std::wstring Wide((size_t)Size, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, Str, -1, Wide.data(), Size);
	Wide.resize((size_t)Size - 1);
	return Wide;
}
#endif

static void RenderProcValue(TwinkTrackmania* Twinkie, CMwStack::ItemType ItemType, const char* Label, ProcCallState::ParamValue& Value, bool ReadOnly)
{
	void* Storage = Value.Storage;

	if (ReadOnly) BeginDisabled();
	switch (ItemType)
	{
	case CMwStack::ITEM_BOOL:
	{
		bool Checked = *(uint32_t*)Storage != 0;
		if (Checkbox(Label, &Checked)) *(uint32_t*)Storage = Checked ? 1U : 0U;
		break;
	}
	case CMwStack::ITEM_ENUM:
	case CMwStack::ITEM_INT:
		InputInt(Label, (int*)Storage);
		break;
	case CMwStack::ITEM_UINT:
		InputScalar(Label, ImGuiDataType_U32, Storage);
		break;
	case CMwStack::ITEM_FLOAT:
		InputFloat(Label, (float*)Storage);
		break;
	case CMwStack::ITEM_VEC2:
		InputFloat2(Label, (float*)Storage);
		break;
	case CMwStack::ITEM_VEC3:
		InputFloat3(Label, (float*)Storage);
		break;
#ifdef TMCN
	case CMwStack::ITEM_INT2:
		InputInt2(Label, (int*)Storage);
		break;
	case CMwStack::ITEM_INT3:
		InputInt3(Label, (int*)Storage);
		break;
	case CMwStack::ITEM_UINT3:
		InputScalarN(Label, ImGuiDataType_U32, Storage, 3);
		break;
	case CMwStack::ITEM_ID:
		InputScalar(Label, ImGuiDataType_U32, Storage, nullptr, nullptr, "0x%08X");
		SameLine();
		TextDisabled("%s", Twinkie->GetMwIdName(*(uint32_t*)Storage).c_str());
		break;
#endif
	case CMwStack::ITEM_ISO4:
		if (TreeNodeEx(Label, ImGuiTreeNodeFlags_DrawLinesFull))
		{
			InputFloat3("X axis", (float*)Storage);
			InputFloat3("Y axis", (float*)Storage + 3);
			InputFloat3("Z axis", (float*)Storage + 6);
			InputFloat3("Position", (float*)Storage + 9);
			TreePop();
		}
		break;
	case CMwStack::ITEM_STRING:
#ifdef TMCN
	case CMwStack::ITEM_WSTRING:
#endif
		if (ReadOnly)
		{
			std::string Shown;
#ifdef TMCN
			if (ItemType == CMwStack::ITEM_WSTRING) Shown = WideToUtf8(((CFastStringInt*)Storage)->CStr, ((CFastStringInt*)Storage)->Count);
			else
#endif
#ifdef TM1
			// TM1 counts the null terminator
			if (((CFastString*)Storage)->CStr) Shown = ((CFastString*)Storage)->CStr;
#else
			if (((CFastString*)Storage)->CStr) Shown.assign(((CFastString*)Storage)->CStr, ((CFastString*)Storage)->Count);
#endif
			EndDisabled();
			InputText(Label, Shown.data(), Shown.size() + 1, ImGuiInputTextFlags_ReadOnly);
			BeginDisabled();
		}
		else
		{
			InputText(Label, Value.Text, sizeof(Value.Text));
		}
		break;
	case CMwStack::ITEM_OBJECT:
		InputTextWithHint(Label, "nod address (hex)", Value.Text, sizeof(Value.Text), ImGuiInputTextFlags_CharsHexadecimal);
		break;
	default:
		TextDisabled("%s: type not supported", Label);
		break;
	}
	if (ReadOnly) EndDisabled();
}

static const char* ProcResultToStr(uint32_t Result)
{
	switch (Result)
	{
	case 0: return "\\$6f6OK";
	case 2: return "\\$f66Invalid arguments (2)";
	case 8: return "\\$f66Script error (8)";
	case UINT_MAX: return "\\$f66Not callable";
	default: return "\\$fa6Unknown result";
	}
}

void NodExplorerModule::RenderProcCall(CMwNod* Nod, CMwMemberInfoProc* Proc, const std::vector<CMwMemberInfoProc::Param>& Params)
{
	ProcCallState& State = ProcCalls[{ Nod, Proc }];
	if (State.Values.size() != Params.size()) State.Values.assign(Params.size(), ProcCallState::ParamValue{});

	bool CanCall = true;
	for (size_t ParamIdx = 0; ParamIdx < Params.size(); ParamIdx++)
	{
		const CMwMemberInfoProc::Param& Param = Params[ParamIdx];
		ProcCallState::ParamValue& Value = State.Values[ParamIdx];
		CMwStack::ItemType ItemType = TwinkTrackmania::GetProcParamItemType(Param.Type);
		bool IsInput = Param.Flags & CMwMemberInfoProc::PARAM_IN;
		std::string Label = std::format("{}{}", Param.Name ? Param.Name : "?", IsInput ? "" : " (out)");

		PushID((int)ParamIdx);
		if (ItemType == CMwStack::ITEM_MEMBER)
		{
			TextDisabled("%s: type not supported", Label.c_str());
			CanCall = false;
		}
		else if (IsInput)
		{
			RenderProcValue(Twinkie, ItemType, Label.c_str(), Value, false);
		}
		else if (!State.HasResult)
		{
			TextDisabled("%s", Label.c_str());
		}
		else if (ItemType == CMwStack::ITEM_OBJECT)
		{
			CMwNod* OutNod = *(CMwNod**)Value.Storage;
			if (OutNod) RenderNod(OutNod, Label, nullptr);
			else TextDisabled("%s: null", Label.c_str());
		}
		else
		{
			RenderProcValue(Twinkie, ItemType, Label.c_str(), Value, true);
		}
		PopID();
	}

	if (!CanCall) BeginDisabled();
	if (Button("Call"))
	{
		std::vector<CMwStack::Item> Args;
		for (size_t ParamIdx = 0; ParamIdx < Params.size(); ParamIdx++)
		{
			const CMwMemberInfoProc::Param& Param = Params[ParamIdx];
			ProcCallState::ParamValue& Value = State.Values[ParamIdx];
			CMwStack::ItemType ItemType = TwinkTrackmania::GetProcParamItemType(Param.Type);
			bool IsInput = Param.Flags & CMwMemberInfoProc::PARAM_IN;
			bool IsOutput = Param.Flags & CMwMemberInfoProc::PARAM_OUT;

			if (IsInput and ItemType == CMwStack::ITEM_OBJECT)
			{
				uintptr_t Address = 0;
				try { if (Value.Text[0]) Address = (uintptr_t)std::stoull(Value.Text, nullptr, 16); }
				catch (...) { Address = 0; }
				*(CMwNod**)Value.Storage = (CMwNod*)Address;
			}
			else if (IsInput and not IsOutput and ItemType == CMwStack::ITEM_STRING)
			{
				Value.NarrowText = Value.Text;
				auto String = (CFastString*)Value.Storage;
				String->CStr = Value.NarrowText.data();
#ifdef TM1
				// TM1 counts the null terminator
				String->Count = (uint32_t)Value.NarrowText.size() + 1;
#else
				String->Count = (uint32_t)Value.NarrowText.size();
#endif
			}
#ifdef TMCN
			else if (IsInput and not IsOutput and ItemType == CMwStack::ITEM_WSTRING)
			{
				Value.WideText = Utf8ToWide(Value.Text);
				auto String = (CFastStringInt*)Value.Storage;
				String->CStr = Value.WideText.data();
				String->Count = (uint32_t)Value.WideText.size();
			}
#endif

			Args.push_back(CMwStack::Item{ Value.Storage, ItemType });
		}

		State.Result = Twinkie->CallProc(Nod, Proc, Args);
		State.HasResult = true;
	}
	if (!CanCall) EndDisabled();

	if (State.HasResult)
	{
		SameLine();
		Text("%s", ProcResultToStr(State.Result));
	}
}

void NodExplorerModule::RenderMenu()
{
	if (MenuItem("NodExplorer", nullptr, Visible))
	{
		Visible = !Visible;
	}
}

void NodExplorerModule::RenderNodInfoClassInfo(CMwClassInfo* ClassInfo)
{
	{
		std::string Copyable = std::format("{:p}", (void*)NodInfo->Nod);
		if (Selectable((std::string("Copy: ") + Copyable + " (nod ptr)").c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		std::string Copyable = ClassInfo->ClassName;
		if (Selectable((std::string("Copy: ") + Copyable).c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		std::string Copyable = std::format("{:x}", ClassInfo->ClassID);
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

void NodExplorerModule::RenderNodInfoMemberInfo()
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
		std::string Copyable = std::format("{:x}", MemberInfo->MemberID);
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
	{
		std::string Copyable = g_MemberTypeNames[(size_t)MemberInfo->MemberType];
		if (Selectable((std::string("Copy: ") + Copyable + "").c_str()))
		{
			SetClipboardText(Copyable.c_str());
			NodInfoPopup = false;
		}
	}
	{
		std::string Copyable = std::format("{:p}", ChildInfo->MemberPtr);
		if (Selectable((std::string("Copy: ") + Copyable + " (ptr to member itself)").c_str()))
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

void NodExplorerModule::RenderInterface()
{
	static struct
	{
		uintptr_t Addr = 0;
		bool IsSafe = false;
	} OtherNod;

	if (!Visible) return;

	Begin("NodExplorer", &Visible, TwinkUiState::DefaultWindowFlags());

	BeginTabBar("NodExplorerTabBar");

	if (BeginTabItem("App", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton))
	{
		RenderNod(Twinkie->GetApp(), "App");
		EndTabItem();
	}

	if (BeginTabItem("Other", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton))
	{
		static uint64_t Temp = 0;
		InputScalar("Address", ImGuiDataType_U64, &Temp, nullptr, nullptr, "%x");
		SameLine();
		if (Button(">"))
		{
			bool PtrSafe = false;
			bool VftableSafe = false;

			PtrSafe = IsAddrSafeSlow(Temp);
			if (PtrSafe) VftableSafe = IsAddrSafeSlow(*(uintptr_t*)Temp);

			bool IsSafe = PtrSafe and VftableSafe;

			if (OtherNod.IsSafe and not IsSafe)
			{
				// do nothing...
			}
			else if (IsSafe)
			{
				OtherNod.Addr = Temp;
				OtherNod.IsSafe = IsSafe;
			}
		}

		if (OtherNod.IsSafe)
		{
			RenderNod((CMwNod*)OtherNod.Addr, "Other");
		}

		EndTabItem();
	}

	EndTabBar();

	End();

	if (NodInfoPopup)
	{
		Begin("##NodExplorerNodInfoPopup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | TwinkUiState::DefaultWindowFlags());

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

		if (NodInfo)
		{
			std::string Copyable = NodInfo->Name;
			if (Selectable((std::string("Copy: ") + Copyable).c_str()))
			{
				SetClipboardText(Copyable.c_str());
				NodInfoPopup = false;
			}

			RenderNodInfoClassInfo(NodInfo->ClassInfo);
		}


		End();
	}
	else if (MemberInfoPopup)
	{
		Begin("##NodExplorerMemberInfoPopup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | TwinkUiState::DefaultWindowFlags());

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

MODULE_EXPORT(NodExplorerModule);