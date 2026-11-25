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
#include <algorithm>
#include <cstring>

// The Trackmania manager class, used by (TwinkTrackmania gTwinkie.TrackmaniaMgr).
class TwinkTrackmania
{
public:
	template <typename ReturnT>
	using GetterReturnT = std::conditional_t<std::is_same_v<ReturnT, CMwNod*>, CMwNod*, ReturnT*>;

	// Where the game puts a virtual member's value. It stays valid until the next virtual get of the same type on this thread
	template<typename ReturnT>
	struct VirtualGetResult
	{
		GetterReturnT<ReturnT> pReturnValue = nullptr;
		uint8_t Storage[sizeof(ReturnT) < 64 ? 64 : sizeof(ReturnT)] = {};
	};

	uintptr_t ExeBaseAddr = 0;

	// Behaviors

	// Ctor
	__declspec(noinline) TwinkTrackmania();
	// Dtor
	__declspec(noinline) virtual ~TwinkTrackmania();


	// Getters

	// Scans for a specific signature (sigscan) in the .exe.
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

	static bool IsVirtualGet(const CMwMemberInfo* MemberInfo)
	{
#ifdef TMCN
		if (MemberInfo->MemberFlags & CMwMemberInfo::VIRTUAL_GET) return true;
#endif
		return MemberInfo->MemberOffset == 0xFFFFFFFFU;
	}

	static bool IsVirtualSet(const CMwMemberInfo* MemberInfo)
	{
#ifdef TMCN
		if (MemberInfo->MemberFlags & CMwMemberInfo::VIRTUAL_SET) return true;
#endif
		return MemberInfo->MemberOffset == 0xFFFFFFFFU;
	}

	// Pushes member to stack.
	TWINKIEX_API void PushToStack(CMwStack* Stack, CMwMemberInfo* MemberInfo);

	// Gets the stack item type a PROC parameter must be passed as. Returns ITEM_MEMBER if the type isn't supported.
	TWINKIEX_API static CMwStack::ItemType GetProcParamItemType(const CMwMemberInfoProc::Type& ParamType);

	// Calls a PROC member on a nod. Args are in the PROC's parameter order, each pointing to storage for that parameter.
	// Output parameters are written through their pointer. Strings written by the game are allocated by the game.
	// In TMCN: Returns the PROC's result: 0 on success, 2 on invalid arguments, 8 on a script error, or UINT_MAX if the member can't be called.
	TWINKIEX_API uint32_t CallProc(CMwNod* Nod, CMwMemberInfo* ProcMember, const std::vector<CMwStack::Item>& Args);

	// Calls a PROC by name. Its first OUT parameter isn't passed: the PROC fills it and it's returned as ReturnT.
	// Returns a default ReturnT if the PROC isn't found, or if its first OUT parameter doesn't have the type of ReturnT.
	template<typename ReturnT = void, typename... TArgs>
	ReturnT CallProc(CMwNod* Nod, const char* Name, TArgs&&... Args);

	// Gets the name of a named MwId from the game's name table, or null if it isn't a valid named ID.
	TWINKIEX_API const char* LookupMwIdName(uint32_t Id);

	// Gets an MwId's name like the game does: the table name, "#<number>" ("Id<number>" on GameBox), "Unassigned" or "Internal Error".
	TWINKIEX_API std::string GetMwIdName(uint32_t Id);

	// Whether a named MwId resolves to a name, or whether any other MwId isn't an internal error.
	TWINKIEX_API bool IsMwIdValid(uint32_t Id);

	// Sets an MwId from a name, adding the name to the game's table if needed.
	TWINKIEX_API void SetMwIdName(uint32_t& Id, const std::string& Name);

	// Gets the game type.
	TWINKIEX_API __declspec(noinline) EGame GetGame();

#ifdef MANIAPLANET
	// Gets the game's DirectX swap chain, only for TMCN/D3D11
	TWINKIEX_API __declspec(noinline) uintptr_t GetDirectXSwapChain();
#endif

private:
#ifdef GAMEBOX
	struct SigOperand
	{
		const char* Sig;
		size_t Offset;
	};

	uintptr_t ReadSigOperand(std::initializer_list<SigOperand> Sigs);
	uintptr_t FindAppGlobal();
	static CMwNod* GetNodMemberById(CMwNod* Nod, uint32_t MemberId);
	static bool IsInDirect3D9Module(uintptr_t Address);
	static bool IsDirectXDevice(uintptr_t Device);
#endif

#ifdef TMCN
	struct MwIdNameChunk
	{
		const uint32_t* Offsets;
		uint32_t Count;
	};

	struct MwIdNameTable
	{
		const char* const* Pool = nullptr;
		const uint32_t* PoolSize = nullptr;
		const MwIdNameChunk* Chunks = nullptr;
		const uint32_t* ChunkCount = nullptr;
	};

	struct MwIdNameView
	{
		const char* Ptr;
		uint32_t Length;
	};
#elif defined(TM1)
	struct MwIdNameTable
	{
		uint32_t Count;
		const CFastString* Names;
		uint32_t Capacity;
	};
#else
	struct MwIdNameChunk
	{
		uint32_t Unknown;
		uint32_t Count;
		const uint32_t* Offsets;
	};

	struct MwIdNameTable
	{
		uint32_t PoolSize;
		const char* Pool;
		uint32_t PoolCapacity;
		MwIdNameChunk Chunks[O_MWID_MAXCHUNKS];
		uint32_t ChunkCount;
	};
#endif

	static bool IsNamedMwId(uint32_t Id);
};

template<typename ReturnT>
TwinkTrackmania::GetterReturnT<typename ReturnT> TwinkTrackmania::VirtualParamGet(CMwNod* Nod, CMwMemberInfo* MemberInfo)
{
	// This variable is used only to not let the compiler optimize the type of Stack->Items for GAMEBOX to a single pointer.
	static CMwStack* PreviousUsedStack = nullptr;

	static thread_local VirtualGetResult<ReturnT> Result;
	Result = {};

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

	static thread_local VirtualGetResult<ReturnT> Result;
	Result = {};

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
	if (IsVirtualGet(MemberInfo) and not IsNodImpersistent)
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

	if (IsVirtualGet(MemberInfo) and not IsNodImpersistent)
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
	if (IsVirtualSet(MemberInfo) and not IsNodImpersistent)
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

	if (IsVirtualSet(MemberInfo) and not IsNodImpersistent)
	{
		return VirtualParamSet(Nod, MemberInfo, Value);
	}
	WriteAddr(ValueT, (uintptr_t)Nod + MemberInfo->MemberOffset, *Value);
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

// How the first OUT parameter of a PROC is stored before being returned as ReturnT
template<typename ReturnT>
struct ProcReturnStorage
{
	using Type = ReturnT;
};

template<>
struct ProcReturnStorage<bool>
{
	using Type = uint32_t;
};

template<>
struct ProcReturnStorage<std::string>
{
	using Type = CFastString;
};

#ifdef TMCN
template<>
struct ProcReturnStorage<std::wstring>
{
	using Type = CFastStringInt;
};
#endif

template<typename ReturnT, typename... TArgs>
ReturnT TwinkTrackmania::CallProc(CMwNod* Nod, const char* Name, TArgs&&... Args)
{
	auto Fail = []() -> ReturnT
	{
		if constexpr (not std::is_void_v<ReturnT>) return ReturnT{};
	};

	if (!Nod) return Fail();

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

	if (!MemberResult or MemberResult->MemberType != CMwMemberInfo::PROC) return Fail();

	auto Params = ((CMwMemberInfoProc*)MemberResult)->GetParams();
	auto OutParam = std::find_if(Params.begin(), Params.end(), [](const CMwMemberInfoProc::Param& Param) { return Param.Flags & CMwMemberInfoProc::PARAM_OUT; });

	CMwStack::ItemType OutType = OutParam != Params.end() ? GetProcParamItemType(OutParam->Type) : CMwStack::ITEM_MEMBER;
	if constexpr (std::is_void_v<ReturnT>)
	{
		if (OutParam != Params.end() and OutType == CMwStack::ITEM_MEMBER) return;
	}
	else
	{
		using StorageT = typename ProcReturnStorage<ReturnT>::Type;

		constexpr CMwStack::ItemType ReturnType = std::is_same_v<ReturnT, bool> ? CMwStack::ITEM_BOOL : TypeToStackItemType<StorageT>();
		if (OutParam == Params.end() or OutType != ReturnType) return Fail();
	}

	// Big enough for any parameter type
	alignas(16) uint8_t OutValue[64] = {};

	std::vector<CMwStack::Item> MadeUpArgs;
	ProcArgStorage Storage;
	MadeUpArgs.reserve(sizeof...(Args) + 1);

	(TToStackItem(MadeUpArgs, Storage, std::forward<TArgs>(Args)), ...);

	if (OutParam != Params.end())
	{
		size_t OutIdx = (size_t)(OutParam - Params.begin());
		if (OutIdx > MadeUpArgs.size()) OutIdx = MadeUpArgs.size();
		MadeUpArgs.insert(MadeUpArgs.begin() + OutIdx, CMwStack::Item{ OutValue, OutType });
	}

	CallProc(Nod, MemberResult, MadeUpArgs);

	if constexpr (not std::is_void_v<ReturnT>)
	{
		using StorageT = typename ProcReturnStorage<ReturnT>::Type;
		StorageT Result;
		std::memcpy(&Result, OutValue, sizeof(StorageT));

		if constexpr (std::is_same_v<ReturnT, bool>)
		{
			return Result != 0;
		}
		else if constexpr (std::is_same_v<ReturnT, std::string>)
		{
			if (!Result.CStr) return {};
#ifdef TM1
			// TM1 counts the null terminator
			return std::string(Result.CStr);
#else
			return std::string(Result.CStr, Result.Count);
#endif
		}
#ifdef TMCN
		else if constexpr (std::is_same_v<ReturnT, std::wstring>)
		{
			return Result.CStr ? std::wstring(Result.CStr, Result.Count) : std::wstring();
		}
#endif
		else
		{
			return Result;
		}
	}
}