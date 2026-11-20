#pragma once

// Offsets for various things
#include <Utils/Offsets.h>

// Various types used by Trackmania
#include <Trackmania/TMTypes.h>

// Useful for realloc
#include <cstdlib>

#include <type_traits>
#include <cassert>
#include <vector>
#include <string>
#include <memory>

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
public:
	template <typename ReturnT>
	using GetterReturnT = std::conditional_t<std::is_same_v<ReturnT, CMwNod*>, CMwNod*, ReturnT*>;

	uintptr_t ExeBaseAddr = 0;

	// Behaviors

	// Ctor
	__declspec(noinline) TwinkTrackmania();
	// Dtor
	__declspec(noinline) virtual ~TwinkTrackmania();


	// Getters

	// Scans for a specific signature (sigscan) in the .exe.
	// The signature MUST have at least 4 bytes, and the first 4 bytes must NOT be a wildcard.
	TWINKIEX_API uintptr_t SigScan(const char* Sig);

	// Gets the main app object, usually defined as (CTrackMania*).
	TWINKIEX_API __declspec(noinline) CMwNod* GetApp();

	// Gets the address to the Viewport of the main app
	TWINKIEX_API __declspec(noinline) CMwNod* GetViewport();

	// Gets the game's DirectX device, as LPDIRECT3DDEVICE9 for GAMEBOX, and ID3D11Device for MANIAPLANET
	TWINKIEX_API __declspec(noinline) uintptr_t GetDirectXDevice();

	// Gets the game's input port, for mouse and keyboard events (always CDx8InputPort or similar)
	TWINKIEX_API __declspec(noinline) CInputPort* GetInputPort();

	// Gets the engine manager that contains all classes
	TWINKIEX_API __declspec(noinline) CMwEngineManager* GetEngineManager();

#ifdef GAMEBOX
	// Sees if the intro when launching the game is over or not. Only for GAMEBOX
	TWINKIEX_API __declspec(noinline) bool GetIsIntroOver();
#endif

	// Not for direct use. Use ParamGet instead.
	template<typename ReturnT>
	GetterReturnT<typename ReturnT> VirtualParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo);

	// Not for direct use. Use ParamGet instead.
	template<typename ReturnT>
	GetterReturnT<typename ReturnT> VirtualParamGet(CMwNod* Nod, const char* MemberName);

	// Not for direct use. Use ParamSet instead.
	template<typename ValueT>
	bool VirtualParamSet(CMwNod* Nod, CMwMemberInfo* MemberInfo, ValueT* Value);

	// Not for direct use. Use ParamSet instead.
	template<typename ValueT>
	bool VirtualParamSet(CMwNod* Nod, const char* MemberName, ValueT* Value);

	// Gets a member's value from a nod. If return result is null, then something failed. Returns pointer to requested datatype unless datatype is CMwNod*, then it returns CMwNod*.
	template<typename ReturnT>
	GetterReturnT<typename ReturnT> ParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo, bool IsNodImpersistent = false);

	// Gets a member's value from a nod. If return result is null, then something failed. Returns pointer to requested datatype unless datatype is CMwNod*, then it returns CMwNod*.
	template<typename ReturnT>
	GetterReturnT<typename ReturnT> ParamGet(CMwNod* Nod, const char* MemberName, bool IsNodImpersistent = false);

	// Sets a member to a value for a nod. Returns true if succeeded.
	template<typename ValueT>
	bool ParamSet(CMwNod* Nod, CMwMemberInfo* MemberInfo, ValueT* Value, bool IsNodImpersistent = false);

	// Sets a member to a value for a nod. Returns true if succeeded.
	template<typename ValueT>
	bool ParamSet(CMwNod* Nod, const char* MemberName, ValueT* Value, bool IsNodImpersistent = false);

	// Pushes member to stack.
	TWINKIEX_API void PushToStack(CMwStack* Stack, CMwMemberInfo* MemberInfo);

	// Gets the stack item type a PROC parameter must be passed as. Returns ITEM_MEMBER if the type isn't supported.
	TWINKIEX_API static CMwStack::ItemType GetProcParamItemType(const CMwMemberInfoProc::Type& ParamType);

	// Calls a PROC member on a nod. Args are in the PROC's parameter order, each pointing to storage for that parameter.
	// Output parameters are written through their pointer. Strings written by the game are allocated by the game.
	// In TMCN: Returns the PROC's result: 0 on success, 2 on invalid arguments, 8 on a script error, or UINT_MAX if the member can't be called.
	TWINKIEX_API uint32_t CallProc(CMwNod* Nod, CMwMemberInfo* ProcMember, const std::vector<CMwStack::Item>& Args);

	template<typename... TArgs>
	uint32_t CallProc(CMwNod* Nod, const char* Name, TArgs&&... Args);

	// Gets the game type.
	TWINKIEX_API __declspec(noinline) EGame GetGame();

#ifdef MANIAPLANET
	// Gets the game's DirectX swap chain, only for TMCN/D3D11
	TWINKIEX_API __declspec(noinline) uintptr_t GetDirectXSwapChain();
#endif
};

template<typename ReturnT>
TwinkTrackmania::GetterReturnT<typename ReturnT> TwinkTrackmania::VirtualParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo)
{
	// This variable is used only to not let the compiler optimize the type of Stack->Items for GAMEBOX to a single pointer.
	static CMwStack* PreviousUsedStack = nullptr;

	struct
	{
		GetterReturnT<ReturnT> pReturnValue = nullptr;
		uint8_t Storage[sizeof(ReturnT)] = {};
	} Result;

	CMwStack Stack = CMwStack();

	PushToStack(&Stack, MemberInfo);

	PreviousUsedStack = &Stack;

	Nod->VirtualParam_Get(&Stack, &Result);

	return Result.pReturnValue;
}

template<typename ReturnT>
TwinkTrackmania::GetterReturnT<typename ReturnT> TwinkTrackmania::VirtualParamGet(CMwNod* Nod, const char* MemberName)
{
	// This variable is used only to not let the compiler optimize the type of Stack->Items for GAMEBOX to a single pointer.
	static CMwStack* PreviousUsedStack = nullptr;

	auto ClassInfo = Nod->MwGetClassInfo();
	CMwMemberInfo* MemberInfo = nullptr;

	while (ClassInfo)
	{ 
		for (auto& Member : *ClassInfo)
		{
			if (strcmp(Member->MemberName, MemberName) == 0)
			{
				MemberInfo = Member;
				break;
			}
		}
		ClassInfo = ClassInfo->ParentClassInfo;
	}

	if (!MemberInfo)
	{
		return nullptr;
	}

	using ReturnTypeT = std::conditional_t<std::is_same_v<ReturnT, CMwNod*>, CMwNod*, ReturnT*>;

	struct
	{
		ReturnTypeT pReturnValue{};
		ReturnT Storage{};
	} Result;

	CMwStack Stack = CMwStack();
	PushToStack(&Stack, MemberInfo);

	PreviousUsedStack = &Stack;

	Nod->VirtualParam_Get(&Stack, &Result);

	return Result.pReturnValue;
}

template<typename ValueT>
bool TwinkTrackmania::VirtualParamSet(CMwNod* Nod, CMwMemberInfo* MemberInfo, ValueT* Value)
{
	CMwStack Stack = CMwStack();
	PushToStack(&Stack, MemberInfo);

	return !Nod->VirtualParam_Set(&Stack, Value);
}

template<typename ValueT>
bool TwinkTrackmania::VirtualParamSet(CMwNod* Nod, const char* MemberName, ValueT* Value)
{
	auto ClassInfo = Nod->MwGetClassInfo();
	CMwMemberInfo* MemberInfo = nullptr;

	while (ClassInfo)
	{
		for (auto& Member : *ClassInfo)
		{
			if (strcmp(Member->MemberName, MemberName) == 0)
			{
				MemberInfo = Member;
				break;
			}
		}
		ClassInfo = ClassInfo->ParentClassInfo;
	}

	if (!MemberInfo)
	{
		return false;
	}

	CMwStack Stack = CMwStack();
	PushToStack(&Stack, MemberInfo);

	return !Nod->VirtualParam_Set(&Stack, Value);
}

template<typename ReturnT>
TwinkTrackmania::GetterReturnT<typename ReturnT> TwinkTrackmania::ParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo, bool IsNodImpersistent)
{
	// TODO: Better virtual member detection here and elsewhere
	if (MemberInfo->MemberOffset == 0xFFFFFFFFU and not IsNodImpersistent)
	{
		return VirtualParamGet<ReturnT>(Nod, MemberInfo);
	}
	if constexpr (not std::is_same_v<ReturnT, CMwNod*>)
	{
		return &ReadAddr(ReturnT, ((uintptr_t)Nod) + MemberInfo->MemberOffset);
	}
	else if (MemberInfo->MemberOffset != 0xFFFFFFFFU)
	{
		return ReadAddr(ReturnT, ((uintptr_t)Nod) + MemberInfo->MemberOffset);
	}
	else
	{
		// TODO: Virtual members for impersistent nods?
		return nullptr;
	}
}

template<typename ReturnT>
TwinkTrackmania::GetterReturnT<typename ReturnT> TwinkTrackmania::ParamGet(CMwNod* Nod, const char* MemberName, bool IsNodImpersistent)
{
	auto ClassInfo = Nod->MwGetClassInfo();
	CMwMemberInfo* MemberInfo = nullptr;

	while (ClassInfo)
	{
		for (auto& Member : *ClassInfo)
		{
			if (strcmp(Member->MemberName, MemberName) == 0)
			{
				MemberInfo = Member;
				break;
			}
		}
		ClassInfo = ClassInfo->ParentClassInfo;
	}

	if (!MemberInfo)
	{
		return nullptr;
	}

	if (MemberInfo->MemberOffset == 0xFFFFFFFFU and not IsNodImpersistent)
	{
		if constexpr (std::is_same_v<CMwNod*, ReturnT>)
		{
			return reinterpret_cast<CMwNod*>(VirtualParamGet<CMwNod*>(Nod, MemberInfo));
		}
		else 
		{
			return VirtualParamGet<ReturnT>(Nod, MemberInfo);
		}
	}
	if constexpr (not std::is_same_v<ReturnT, CMwNod*>)
	{
		return &ReadAddr(ReturnT, ((uintptr_t)Nod) + MemberInfo->MemberOffset);
	}
	else
	{
		return ReadAddr(ReturnT, ((uintptr_t)Nod) + MemberInfo->MemberOffset);
	}
}

template<typename ValueT>
bool TwinkTrackmania::ParamSet(CMwNod* Nod, CMwMemberInfo* MemberInfo, ValueT* Value, bool IsNodImpersistent)
{
	if (MemberInfo->MemberOffset == 0xFFFFFFFFU and not IsNodImpersistent)
	{
		return VirtualParamSet(Nod, MemberInfo, Value);
	}
	WriteAddr(ValueT, (uintptr_t)Nod + MemberInfo->MemberOffset, *Value);
	return true;
}

template<typename ValueT>
bool TwinkTrackmania::ParamSet(CMwNod* Nod, const char* MemberName, ValueT* Value, bool IsNodImpersistent)
{
	auto ClassInfo = Nod->MwGetClassInfo();
	CMwMemberInfo* MemberInfo = nullptr;

	while (ClassInfo)
	{
		for (auto& Member : *ClassInfo)
		{
			if (strcmp(Member->MemberName, MemberName) == 0)
			{
				MemberInfo = Member;
				break;
			}
		}
		ClassInfo = ClassInfo->ParentClassInfo;
	}

	if (!MemberInfo)
	{
		return false;
	}

	if (MemberInfo->MemberOffset == 0xFFFFFFFFU and not IsNodImpersistent)
	{
		return VirtualParamSet(Nod, MemberInfo, Value);
	}
	WriteAddr(ValueT, (uintptr_t)Nod + MemberInfo, *Value);
	return true;
}

// Keeps the values passed to a PROC alive until the PROC returns
using ProcArgStorage = std::vector<std::shared_ptr<void>>;

template<typename T>
void* TToStackItem(std::vector<CMwStack::Item>& Args, ProcArgStorage& Storage, T&& Arg)
{
	using ArgT = std::remove_cvref_t<T>;

	auto Push = [&]<typename ValueT>(ValueT Value, CMwStack::ItemType Type) -> void*
	{
		auto Copy = std::make_shared<ValueT>(Value);
		Storage.push_back(Copy);
		Args.push_back(CMwStack::Item{ Copy.get(), Type });
		return Copy.get();
	};

	if constexpr (std::is_same_v<ArgT, bool>)
	{
		return Push((uint32_t)(Arg ? 1 : 0), CMwStack::ITEM_BOOL);
	}
	else if constexpr (std::is_enum_v<ArgT>)
	{
		return Push((uint32_t)Arg, CMwStack::ITEM_ENUM);
	}
	else if constexpr (std::is_null_pointer_v<ArgT>)
	{
		return Push((CMwNod*)nullptr, CMwStack::ITEM_OBJECT);
	}
	else if constexpr (std::is_convertible_v<ArgT, const char*> or std::is_same_v<ArgT, std::string>)
	{
		struct Holder
		{
			std::string Text;
			CFastString String{};
		};
		auto Copy = std::make_shared<Holder>();
		if constexpr (std::is_same_v<ArgT, std::string>) Copy->Text = Arg;
		else if (Arg) Copy->Text = Arg;

		Copy->String.CStr = Copy->Text.data();
#ifdef TM1
		Copy->String.Count = (uint32_t)Copy->Text.size() + 1;
#else
		Copy->String.Count = (uint32_t)Copy->Text.size();
#endif
		Storage.push_back(Copy);
		Args.push_back(CMwStack::Item{ &Copy->String, CMwStack::ITEM_STRING });
		return &Copy->String;
	}
#ifdef TMCN
	else if constexpr (std::is_convertible_v<ArgT, const wchar_t*> or std::is_same_v<ArgT, std::wstring>)
	{
		struct Holder
		{
			std::wstring Text;
			CFastStringInt String{};
		};
		auto Copy = std::make_shared<Holder>();
		if constexpr (std::is_same_v<ArgT, std::wstring>) Copy->Text = Arg;
		else if (Arg) Copy->Text = Arg;

		Copy->String.CStr = Copy->Text.data();
		Copy->String.Count = (uint32_t)Copy->Text.size();
		Storage.push_back(Copy);
		Args.push_back(CMwStack::Item{ &Copy->String, CMwStack::ITEM_WSTRING });
		return &Copy->String;
	}
#endif
	else if constexpr (std::is_pointer_v<ArgT> and std::is_base_of_v<CMwNod, std::remove_pointer_t<ArgT>>)
	{
		return Push((CMwNod*)Arg, CMwStack::ITEM_OBJECT);
	}
	else if constexpr (std::is_pointer_v<ArgT>)
	{
		using PointeeT = std::remove_pointer_t<ArgT>;

		Args.push_back(CMwStack::Item{ (void*)Arg, TypeToStackItemType<PointeeT>() });
		return (void*)Arg;
	}
	else
	{
		return Push((ArgT)Arg, TypeToStackItemType<ArgT>());
	}
}

template<typename... TArgs>
uint32_t TwinkTrackmania::CallProc(CMwNod* Nod, const char* Name, TArgs&&... Args)
{
	if (!Nod) return UINT_MAX;

	CMwMemberInfo* MemberResult = nullptr;
	for (auto ClassInfo = Nod->MwGetClassInfo(); ClassInfo and not MemberResult; ClassInfo = ClassInfo->ParentClassInfo)
	{
		for (auto& Member : *ClassInfo)
		{
			if (Member->GetName() == Name)
			{
				MemberResult = Member;
				break;
			}
		}
	}

	if (!MemberResult) return UINT_MAX;

	std::vector<CMwStack::Item> MadeUpArgs;
	ProcArgStorage Storage;
	MadeUpArgs.reserve(sizeof...(Args));

	(TToStackItem(MadeUpArgs, Storage, std::forward<TArgs>(Args)), ...);

	return CallProc(Nod, MemberResult, MadeUpArgs);
}