// Precompiled headers.
#include "pch.h"

#include <windows.h>

// Class definition for the TwinkTrackmania class, that also includes extra headers for Trackmania.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Behaviors

TwinkTrackmania::TwinkTrackmania()
{
	this->ExeBaseAddr = (uintptr_t)GetModuleHandleA(NULL);

#ifdef TMCN
	// This is for removing the timelimit entirely.
	Unprotect(this->ExeBaseAddr + 0x84976c, 8);
	WriteAddr(uint64_t, this->ExeBaseAddr + 0x84976c, 0x3B'F0'8B'90'90'90'90'90);
#endif
}

TwinkTrackmania::~TwinkTrackmania()
{
}

// END Behaviors

// Getters

uintptr_t TwinkTrackmania::SigScan(const char* Sig)
{
	__pragma(pack(push, 1))
	struct OptionU8
	{
		bool IsWildcard;
		uint8_t Byte;
		
		bool operator==(uint8_t Rhs)
		{
			if (IsWildcard) return true;
			return Rhs == Byte;
		}
	};
	__pragma(pack(pop))

	std::vector<OptionU8> SigBytes;
	std::string SigStr = Sig;
	std::string Byte;

	for (auto& Char : SigStr)
	{
		if (Char == ' ')
		{
			if (Byte[0] == '?')
			{
				SigBytes.push_back({ true, 0 });
			}
			else
			{
				SigBytes.push_back({ false, (uint8_t)std::stoi(Byte, nullptr, 16) });
			}
			Byte = "";
		}
		else
		{
			Byte += Char;
		}
	}

	if (Byte[0] == '?')
	{
		SigBytes.push_back({ true, 0 });
	}
	else
	{
		SigBytes.push_back({ false, (uint8_t)std::stoi(Byte, nullptr, 16) });
	}

	uintptr_t Start = this->ExeBaseAddr;

	// https://wiki.alliedmods.net/Signature_Scanning
	MEMORY_BASIC_INFORMATION Mbi;

	if (!VirtualQuery((const void*)Start, &Mbi, sizeof(Mbi)))
		return 0;

	IMAGE_DOS_HEADER* DosHeader = (IMAGE_DOS_HEADER*)Mbi.AllocationBase;

#ifdef X64
#define NtHeadersStruct IMAGE_NT_HEADERS64
#else
#define NtHeadersStruct IMAGE_NT_HEADERS
#endif

	NtHeadersStruct* PeHeader = (NtHeadersStruct*)((uintptr_t)DosHeader + (uintptr_t)DosHeader->e_lfanew);

	uintptr_t ImageSize = (uintptr_t)PeHeader->OptionalHeader.SizeOfImage;
	uintptr_t End = Start + ImageSize;
	
	uintptr_t Current = Start;

	uint32_t FirstFourBytesOfSig =
		((uint32_t)SigBytes[0].Byte << 0 ) |
		((uint32_t)SigBytes[1].Byte << 8 ) |
		((uint32_t)SigBytes[2].Byte << 16) |
		((uint32_t)SigBytes[3].Byte << 24) ;

	bool MatchedFirstFourBytes = false;

	while (Current + SigBytes.size() <= End)
	{
		while (Current + SigBytes.size() <= End)
		{
			if (ReadAddr(uint32_t, Current) == FirstFourBytesOfSig)
			{
				MatchedFirstFourBytes = true;
				break;
			}
			Current++;
		}

		if (MatchedFirstFourBytes)
		{
			uintptr_t CurrentForSearch = Current + sizeof(uint32_t);
			while (CurrentForSearch - Current < SigBytes.size())
			{
				size_t Idx = CurrentForSearch - Current;
				if (ReadAddr(uint8_t, CurrentForSearch) != SigBytes[Idx])
				{
					Current++;
					MatchedFirstFourBytes = false;
					break;
				}
				CurrentForSearch++;
			}

		}
		
		if (MatchedFirstFourBytes)
		{
			return Current;
		}
	}

	return 0;
}

CMwNod* TwinkTrackmania::GetApp()
{
#ifndef TM1
	return ReadAddr(CMwNod*, this->ExeBaseAddr + O_APP);
#else
	return ReadAddr(CMwNod*, O_M_CMWNODMAIN_APP + O_APP);
#endif
}

CMwNod* TwinkTrackmania::GetViewport()
{
#ifndef TM1
	return ReadAddr(CMwNod*, (uintptr_t)GetApp() + O_M_CTRACKMANIA_VIEWPORT);
#else
	return ReadAddr(CMwNod*, O_M_CMWNODMAIN_VIEWPORT + O_APP);
#endif
}

uintptr_t TwinkTrackmania::GetDirectXDevice()
{
#ifndef TM1
	return ReadAddr(uintptr_t, this->ExeBaseAddr + O_D3DDEVICE);
#else
	return ReadAddr(uintptr_t, O_M_CVISIONVIEWPORTDX9_DEVICE + (uintptr_t)GetViewport());
#endif
}

CInputPort* TwinkTrackmania::GetInputPort()
{
	return ReadAddr(CInputPort*, (uintptr_t)GetApp() + O_M_CTRACKMANIA_INPUTPORT);
}

CMwEngineManager* TwinkTrackmania::GetEngineManager()
{
	return (CMwEngineManager*)(this->ExeBaseAddr + O_ENGINEMGR);
}

#ifdef GAMEBOX
bool TwinkTrackmania::GetIsIntroOver()
{
#ifndef TMU
	return ReadAddr(bool, this->ExeBaseAddr + O_ISINTROOVER);
#else
	return true;
#endif
}
#endif

#ifdef MANIAPLANET
uintptr_t TwinkTrackmania::GetDirectXSwapChain()
{
	return ReadAddr(uintptr_t, (uintptr_t)this->GetViewport() + O_M_CDX11VIEWPORT_D3DSWAPCHAIN);
}


#endif

void TwinkTrackmania::PushToStack(CMwStack* Stack, CMwMemberInfo* MemberInfo)
{
#ifdef GAMEBOX
	if (Stack->Size == Stack->Capacity)
	{
		uint32_t Capacity = Stack->Capacity ? Stack->Capacity * 2 : 2;
		auto Items = (void**)realloc(Stack->Items, Capacity * sizeof(void*));
		if (Items) Stack->Items = Items;
		auto Types = (CMwStack::ItemType*)realloc(Stack->Types, Capacity * sizeof(CMwStack::ItemType));
		if (Types) Stack->Types = Types;
		if (!Items or !Types) return;
		Stack->Capacity = Capacity;
	}

	// Like the game, new items go at the start: the game reads from the end of the stack towards the start
	memmove(Stack->Items + 1, Stack->Items, Stack->Size * sizeof(void*));
	memmove(Stack->Types + 1, Stack->Types, Stack->Size * sizeof(CMwStack::ItemType));
	Stack->Items[0] = MemberInfo;
	Stack->Types[0] = CMwStack::ITEM_MEMBER;
	Stack->Size++;
	Stack->CurrentPos = (int32_t)Stack->Size - 1;
#else
	if (Stack->Size < 2)
	{
		Stack->ContainedItems[Stack->Size] = CMwStack::Item{ MemberInfo, CMwStack::ITEM_MEMBER };
		Stack->Size++;
	}
	else
	{
		if (Stack->ExtraItemsCapacity != 0 and Stack->ExtraItems)
		{
			Stack->ExtraItems = (CMwStack::Item*)realloc(Stack->ExtraItems, (Stack->ExtraItemsCapacity + 1) * sizeof(CMwStack::Item));
		}
		else if (Stack->ExtraItemsCapacity == 0)
		{
			if (Stack->ExtraItems) free(Stack->ExtraItems);
			Stack->ExtraItems = (CMwStack::Item*)malloc((Stack->ExtraItemsCapacity + 1) * sizeof(CMwStack::Item));
		}
		if (Stack->ExtraItems) Stack->ExtraItems[Stack->ExtraItemsCapacity] = CMwStack::Item{ MemberInfo, CMwStack::ITEM_MEMBER };
		Stack->ExtraItemsCapacity++;
		Stack->Size++;
	}
#endif
}

CMwStack::ItemType TwinkTrackmania::GetProcParamItemType(const CMwMemberInfoProc::Type& ParamType)
{
	switch (ParamType.Kind)
	{
	case CMwMemberInfoProc::Type::CLASS:
		return CMwStack::ITEM_OBJECT;
	case CMwMemberInfoProc::Type::ENUM:
		return CMwStack::ITEM_ENUM;
	case CMwMemberInfoProc::Type::VALUE:
		switch (ParamType.AdditionalInfo)
		{
		case CMwMemberInfo::BOOL:      return CMwStack::ITEM_BOOL;
		case CMwMemberInfo::ENUM:      return CMwStack::ITEM_ENUM;
		case CMwMemberInfo::INT:       return CMwStack::ITEM_INT;
		case CMwMemberInfo::NATURAL:   return CMwStack::ITEM_UINT;
		case CMwMemberInfo::REAL:      return CMwStack::ITEM_FLOAT;
		case CMwMemberInfo::ISO4:      return CMwStack::ITEM_ISO4;
		case CMwMemberInfo::VEC2:      return CMwStack::ITEM_VEC2;
		case CMwMemberInfo::VEC3:      return CMwStack::ITEM_VEC3;
		case CMwMemberInfo::STRING:    return CMwStack::ITEM_STRING;
#ifdef TMCN
		case CMwMemberInfo::INT2:      return CMwStack::ITEM_INT2;
		case CMwMemberInfo::INT3:      return CMwStack::ITEM_INT3;
		case CMwMemberInfo::NAT3:      return CMwStack::ITEM_UINT3;
		case CMwMemberInfo::STRINGINT: return CMwStack::ITEM_WSTRING;
		case CMwMemberInfo::ID:        return CMwStack::ITEM_ID;
#endif
		default:                       return CMwStack::ITEM_MEMBER;
		}
	}
	return CMwStack::ITEM_MEMBER;
}

#ifdef TMCN
uint32_t TwinkTrackmania::CallProc(CMwNod* Nod, CMwMemberInfo* ProcMember, const std::vector<CMwStack::Item>& Args)
{
	if (!Nod || !ProcMember || ProcMember->MemberType != CMwMemberInfo::PROC) return UINT_MAX;

	auto Proc = (CMwMemberInfoProc*)ProcMember;
	CMwMemberInfoProc::ProcFn Method = Proc->MethodItself ? Proc->MethodItself : Proc->MethodAlt;
	if (!Method) return UINT_MAX;

	size_t ItemCount = Args.size() + 1;
	if (ItemCount > 0xFF) return UINT_MAX;

	std::vector<CMwStack::Item> Items;
	Items.reserve(ItemCount);
	Items.push_back(CMwStack::Item{ ProcMember, CMwStack::ITEM_MEMBER });
	for (auto Arg = Args.rbegin(); Arg != Args.rend(); ++Arg) Items.push_back(*Arg);

	CMwStack Stack{};
	Stack.Size = (uint8_t)ItemCount;
	Stack.CurrentPos = 1;
	for (size_t ItemIdx = 0; ItemIdx < ItemCount and ItemIdx < 2; ItemIdx++)
	{
		Stack.ContainedItems[ItemIdx] = Items[ItemIdx];
	}
	if (ItemCount > 2)
	{
		Stack.ExtraItems = Items.data() + 2;
		Stack.ExtraItemsCapacity = (uint8_t)(ItemCount - 2);
	}

	return Method(Nod, &Stack);
}
#else
uint32_t TwinkTrackmania::CallProc(CMwNod* Nod, CMwMemberInfo* ProcMember, const std::vector<CMwStack::Item>& Args)
{
	if (!Nod || !ProcMember || ProcMember->MemberType != CMwMemberInfo::PROC) return UINT_MAX;

	auto Proc = (CMwMemberInfoProc*)ProcMember;
	bool IsVirtual = Proc->GetFlags() & CMwMemberInfo::VIRTUAL_SET;
	if (!IsVirtual and !Proc->Method) return UINT_MAX;

	CMwStack Stack;
	Stack.Size = (uint32_t)Args.size() + 1;
	Stack.Capacity = Stack.Size;
	Stack.Items = (void**)malloc(Stack.Capacity * sizeof(void*));
	Stack.Types = (CMwStack::ItemType*)malloc(Stack.Capacity * sizeof(CMwStack::ItemType));
	if (!Stack.Items or !Stack.Types) return UINT_MAX;

	// The arguments go first in the PROC's parameter order, then the PROC: the game reads the PROC from the end and the arguments from the start
	for (size_t ArgIdx = 0; ArgIdx < Args.size(); ArgIdx++)
	{
		Stack.Items[ArgIdx] = Args[ArgIdx].Value;
		Stack.Types[ArgIdx] = Args[ArgIdx].Type;
	}
	Stack.Items[Args.size()] = ProcMember;
	Stack.Types[Args.size()] = CMwStack::ITEM_MEMBER;

	// Same as CMwNod::Param_Set: virtual PROCs get the cursor on the PROC, the others get it past the PROC
	if (IsVirtual)
	{
		Stack.CurrentPos = (int32_t)Stack.Size - 1;
		return Nod->VirtualParam_Set(&Stack, nullptr);
	}

	Stack.CurrentPos = (int32_t)Stack.Size - 2;
	Proc->Method(Nod, &Stack);
	return 0;
}
#endif

__declspec(noinline) EGame TwinkTrackmania::GetGame()
{
	return TWINKIEX_GAMEENUM;
}

// END Getters

namespace
{
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

	constexpr uint32_t MaxMwIdNameChunks = 32;
#elif defined(TM1)
	struct MwIdNameTable
	{
		uint32_t Count;
		const CFastString* Names;
		uint32_t Capacity;
	};
#else
	constexpr uint32_t MaxMwIdNameChunks = 32;

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
		MwIdNameChunk Chunks[MaxMwIdNameChunks];
		uint32_t ChunkCount;
	};
	static_assert(offsetof(MwIdNameTable, ChunkCount) == 0x18C);
#endif

	bool IsNamedMwId(uint32_t Id)
	{
		uint32_t Top = Id & 0xC0000000;
#ifdef TMCN
		return Top == 0x40000000;
#else
		return Top == 0x40000000 or Top == 0x80000000;
#endif
	}
}

const char* TwinkTrackmania::LookupMwIdName(uint32_t Id)
{
#ifdef TMCN
	static MwIdNameTable Table = [this]
	{
		MwIdNameTable Found{};
		auto Lookup = (const uint8_t*)SigScan("8B 11 8B C2 25 00 00 00 C0 3D 00 00 00 40 75 ?? 81 E2 FF FF FF 3F 4C 8D 05 ?? ?? ?? ?? 8B C2 0F B7 CA 48 C1 E8 10 48 03 C0 49 8B 04 C0 8B 04 88 48 03 05");
		if (!Lookup) return Found;

		const uint8_t* Chunks = Lookup + 0x1D + *(const int32_t*)(Lookup + 0x19);
		const uint8_t* Pool = Lookup + 0x37 + *(const int32_t*)(Lookup + 0x33);
		Found.Pool = (const char* const*)Pool;
		Found.PoolSize = (const uint32_t*)(Pool + sizeof(void*));
		Found.Chunks = (const MwIdNameChunk*)Chunks;
		Found.ChunkCount = (const uint32_t*)(Chunks + MaxMwIdNameChunks * sizeof(MwIdNameChunk));
		return Found;
	}();

	if (!Table.Chunks or !IsNamedMwId(Id)) return nullptr;

	uint32_t Index = Id & 0x3FFFFFFF;
	uint32_t ChunkIdx = Index >> 16;
	uint32_t Slot = Index & 0xFFFF;
	if (ChunkIdx >= MaxMwIdNameChunks or ChunkIdx >= *Table.ChunkCount) return nullptr;

	const MwIdNameChunk& Chunk = Table.Chunks[ChunkIdx];
	if (!Chunk.Offsets or Slot >= Chunk.Count) return nullptr;

	uint32_t Offset = Chunk.Offsets[Slot];
	const char* Pool = *Table.Pool;
	if (Offset == 0xFFFFFFFF or !Pool or Offset >= *Table.PoolSize) return nullptr;
	return Pool + Offset;
#elif defined(TM1)
	static auto TablePtr = [this]() -> MwIdNameTable* const*
	{
		auto Lookup = (const uint8_t*)SigScan("8B 01 8B C8 81 E1 00 00 00 C0 81 F9 00 00 00 40 74 03 33 C0 C3 25 FF FF FF 3F 8D 14 40 A1");
		return Lookup ? *(MwIdNameTable* const**)(Lookup + 0x1E) : nullptr;
	}();

	if (!TablePtr or !*TablePtr or !IsNamedMwId(Id)) return nullptr;

	const MwIdNameTable* Table = *TablePtr;
	uint32_t Index = Id & 0x3FFFFFFF;
	if (!Table->Names or Index >= Table->Count) return nullptr;
	return Table->Names[Index].CStr;
#else
	static auto TablePtr = [this]() -> MwIdNameTable* const*
	{
		auto Lookup = (const uint8_t*)SigScan("8B 01 8B C8 81 E1 00 00 00 C0 81 F9 00 00 00 40 74 0B 81 F9 00 00 00 80 74 03 33 C0 C3 25 FF FF FF 3F");
		if (!Lookup) return nullptr;

		for (uint32_t Offset = 0x22; Offset < 0x40; Offset++)
		{
			if (Lookup[Offset] == 0x8B and (Lookup[Offset + 1] == 0x0D or Lookup[Offset + 1] == 0x35))
				return *(MwIdNameTable* const**)(Lookup + Offset + 2);
		}
		return nullptr;
	}();

	if (!TablePtr or !*TablePtr or !IsNamedMwId(Id)) return nullptr;

	const MwIdNameTable* Table = *TablePtr;
	uint32_t Index = Id & 0x3FFFFFFF;
	uint32_t ChunkIdx = Index >> 16;
	uint32_t Slot = Index & 0xFFFF;
	if (ChunkIdx >= MaxMwIdNameChunks or ChunkIdx >= Table->ChunkCount) return nullptr;

	const MwIdNameChunk& Chunk = Table->Chunks[ChunkIdx];
	if (!Chunk.Offsets or Slot >= Chunk.Count) return nullptr;

	uint32_t Offset = Chunk.Offsets[Slot];
	if (Offset == 0xFFFFFFFF or !Table->Pool or Offset >= Table->PoolSize) return nullptr;
	return Table->Pool + Offset;
#endif
}

std::string TwinkTrackmania::GetMwIdName(uint32_t Id)
{
	if (IsNamedMwId(Id))
	{
		const char* Name = LookupMwIdName(Id);
		return Name ? Name : "";
	}

	switch (Id & 0xC0000000)
	{
	case 0x00000000:
#ifdef TMCN
		return std::format("#{}", Id);
#else
		return std::format("Id{}", Id);
#endif
	case 0x80000000:
		return "Internal Error";
	default:
		return "Unassigned";
	}
}

bool TwinkTrackmania::IsMwIdValid(uint32_t Id)
{
	if (IsNamedMwId(Id)) return LookupMwIdName(Id) != nullptr;
	return (Id & 0xC0000000) != 0x80000000;
}

void TwinkTrackmania::SetMwIdName(uint32_t& Id, const std::string& Name)
{
#ifdef TMCN
	using SetNameFn = void (*)(uint32_t*, const MwIdNameView*);
	static auto Set = (SetNameFn)SigScan("48 89 5C 24 08 57 48 83 EC 30 0F 28 02 48 8B D9 48 8D 4C 24 20 66 0F 7F 44 24 20 48 8B FA E8 ?? ?? ?? ?? 85 C0 74 ?? C7 03 FF FF FF FF");
	if (!Set) return;

	alignas(16) MwIdNameView View{ Name.c_str(), (uint32_t)Name.size() };
	Set(&Id, &View);
#else
	using NameToIndexFn = uint32_t(__cdecl*)(const char*);
	static auto NameToIndex = [this]() -> NameToIndexFn
	{
		auto Call = (const uint8_t*)SigScan("B9 0B 00 00 00 33 D2 F3 A6 5F 5E 74 ?? 50 E8");
		if (!Call) return nullptr;
		return (NameToIndexFn)(Call + 0x13 + *(const int32_t*)(Call + 0x0F));
	}();
	if (!NameToIndex) return;

	if (Name.c_str()[0] == '\0' or std::strcmp(Name.c_str(), "Unassigned") == 0)
	{
		Id = 0xFFFFFFFF;
		return;
	}
	Id = NameToIndex(Name.c_str()) | 0x40000000;
#endif
}
