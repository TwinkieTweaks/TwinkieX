#include "pch.h"
#include "Dev.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/Exports/Math/Math.h>
#include <Gecko/Exports/Trackmania/Trackmania.h>
#include <Twinkie/Twinkie.h>
#include <cctype>
#include <sstream>

namespace Gecko::Exports::Dev
{
	using namespace Gecko::Exports::Math;
	using Gecko::Exports::Trackmania::ClassNameToInfo;

	static std::vector<GeckoExport> s_Exports;
#define DEV_FN(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "Dev", Decl_, Fn_)
#define FORCECAST_BEHAVIOUR(Beh_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "Dev", "ForceCast<T>", Beh_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define FORCECAST_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "Dev", "ForceCast<T>", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	static void Throw(const std::string& Message)
	{
		if (auto Ctx = asGetActiveContext()) Ctx->SetException(Message.c_str());
	}

	static std::string MemoryError(const char* What, uint64_t Ptr)
	{
		return std::format("Couldn't {} memory at 0x{:X}", What, Ptr);
	}

	static void* AsPtr(uint64_t Ptr)
	{
		return (void*)(uintptr_t)Ptr;
	}

	static std::string ToHex(const uint8_t* Bytes, size_t Size)
	{
		static constexpr char Digits[] = "0123456789ABCDEF";
		std::string Hex;
		Hex.reserve(Size * 3);
		for (size_t Idx = 0; Idx < Size; Idx++)
		{
			if (Idx) Hex += ' ';
			Hex += Digits[Bytes[Idx] >> 4];
			Hex += Digits[Bytes[Idx] & 0xF];
		}
		return Hex;
	}

	static bool ParsePattern(const std::string& Pattern, std::vector<int16_t>& Bytes)
	{
		std::istringstream Stream(Pattern);
		std::string Token;
		while (Stream >> Token)
		{
			if (Token == "?" or Token == "??")
			{
				Bytes.push_back(-1);
				continue;
			}
			if (Token.size() != 2 or !std::isxdigit((uint8_t)Token[0]) or !std::isxdigit((uint8_t)Token[1]))
			{
				Throw("Invalid byte in pattern: " + Token);
				return false;
			}
			Bytes.push_back((int16_t)std::stoi(Token, nullptr, 16));
		}
		if (Bytes.empty())
		{
			Throw("Empty pattern");
			return false;
		}
		return true;
	}

	static bool TryCopy(void* Dst, const void* Src, size_t Size)
	{
		__try
		{
			memcpy(Dst, Src, Size);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	static uintptr_t ImageEnd()
	{
		uintptr_t Base = gTwinkie.TrackmaniaMgr.ExeBaseAddr;
		auto Dos = (const IMAGE_DOS_HEADER*)Base;
		auto Nt = (const IMAGE_NT_HEADERS*)(Base + Dos->e_lfanew);
		return Base + Nt->OptionalHeader.SizeOfImage;
	}

	static void* ObjectFrom(void* Ref, int TypeId)
	{
		if (!Ref or TypeId == asTYPEID_VOID) return nullptr;
		if (TypeId & asTYPEID_OBJHANDLE) return *(void**)Ref;
		return Ref;
	}

	static uint8_t* NodFrom(void* Ref, int TypeId)
	{
		asIScriptContext* Ctx = asGetActiveContext();
		asITypeInfo* Type = Ctx ? Ctx->GetEngine()->GetTypeInfoById(TypeId) : nullptr;
		if (!Type or !ClassNameToInfo.contains(Type->GetName()))
		{
			Throw("Expected a nod");
			return nullptr;
		}

		auto Nod = (uint8_t*)ObjectFrom(Ref, TypeId);
		if (!Nod) Throw("Null nod");
		return Nod;
	}

	static void Dev_Sleep(uint32_t Ms)
	{
		::Sleep(Ms);
	}

	static uint64_t BaseAddress()
	{
		return gTwinkie.TrackmaniaMgr.ExeBaseAddr;
	}

	static uint64_t BaseAddressEnd()
	{
		return ImageEnd();
	}

	static uint64_t FindPattern(const std::string& Pattern)
	{
		std::vector<int16_t> Bytes;
		if (!ParsePattern(Pattern, Bytes)) return 0;

		size_t Anchor = 0;
		while (Anchor < Bytes.size() and Bytes[Anchor] < 0) Anchor++;

		auto Matches = [&](const uint8_t* At)
		{
			for (size_t Idx = 0; Idx < Bytes.size(); Idx++)
			{
				if (Bytes[Idx] >= 0 and At[Idx] != (uint8_t)Bytes[Idx]) return false;
			}
			return true;
		};

		auto ScanSpan = [&](uintptr_t Begin, uintptr_t End) -> uintptr_t
		{
			if (End - Begin < Bytes.size()) return 0;
			auto Cur = (const uint8_t*)Begin;
			auto Last = (const uint8_t*)(End - Bytes.size());
			while (Cur <= Last)
			{
				if (Anchor < Bytes.size())
				{
					auto Found = (const uint8_t*)memchr(Cur + Anchor, (uint8_t)Bytes[Anchor], (size_t)(Last - Cur) + 1);
					if (!Found) return 0;
					Cur = Found - Anchor;
				}
				if (Matches(Cur)) return (uintptr_t)Cur;
				Cur++;
			}
			return 0;
		};

		uintptr_t End = ImageEnd();
		uintptr_t Addr = gTwinkie.TrackmaniaMgr.ExeBaseAddr;
		uintptr_t SpanBegin = 0;
		while (Addr < End)
		{
			MEMORY_BASIC_INFORMATION Mbi{};
			if (!VirtualQuery((const void*)Addr, &Mbi, sizeof(Mbi))) break;

			uintptr_t RegionEnd = (std::min)((uintptr_t)Mbi.BaseAddress + (uintptr_t)Mbi.RegionSize, End);
			bool Readable = Mbi.State == MEM_COMMIT and !(Mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
			if (Readable and !SpanBegin) SpanBegin = Addr;
			if (!Readable and SpanBegin)
			{
				if (uintptr_t Found = ScanSpan(SpanBegin, Addr)) return Found;
				SpanBegin = 0;
			}
			Addr = RegionEnd;
		}
		return SpanBegin ? ScanSpan(SpanBegin, Addr) : 0;
	}

	static std::string Patch(uint64_t Ptr, const std::string& Pattern)
	{
		std::vector<int16_t> Bytes;
		if (!ParsePattern(Pattern, Bytes)) return {};

		auto Addr = (uint8_t*)AsPtr(Ptr);
		DWORD OldProtect = 0;
		if (!VirtualProtect(Addr, Bytes.size(), PAGE_EXECUTE_READWRITE, &OldProtect))
		{
			Throw(MemoryError("unprotect", Ptr));
			return {};
		}

		std::string Original = ToHex(Addr, Bytes.size());
		for (size_t Idx = 0; Idx < Bytes.size(); Idx++)
		{
			if (Bytes[Idx] >= 0) Addr[Idx] = (uint8_t)Bytes[Idx];
		}

		VirtualProtect(Addr, Bytes.size(), OldProtect, &OldProtect);
		FlushInstructionCache(GetCurrentProcess(), Addr, Bytes.size());
		return Original;
	}

	static uint64_t Allocate(uint32_t Size, bool Executable)
	{
		return (uintptr_t)VirtualAlloc(nullptr, Size, MEM_COMMIT | MEM_RESERVE, Executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
	}

	static void Free(uint64_t Ptr)
	{
		if (Ptr) VirtualFree(AsPtr(Ptr), 0, MEM_RELEASE);
	}

	static void Dev_DebugBreak()
	{
		if (IsDebuggerPresent()) __debugbreak();
	}

	static std::string Read(uint64_t Ptr, uint64_t Size)
	{
		return ToHex((const uint8_t*)AsPtr(Ptr), (size_t)Size);
	}

	template<typename T>
	static T ReadT(uint64_t Ptr)
	{
		T Value{};
		memcpy(&Value, AsPtr(Ptr), sizeof(T));
		return Value;
	}

	static std::string ReadCStringLength(uint64_t Ptr, uint32_t Length)
	{
		auto Str = (const char*)AsPtr(Ptr);
		return std::string(Str, strnlen(Str, Length));
	}

	static std::string ReadCString(uint64_t Ptr)
	{
		return std::string((const char*)AsPtr(Ptr));
	}

	static std::string SafeRead(uint64_t Ptr, uint64_t Size)
	{
		std::vector<uint8_t> Buffer((size_t)Size);
		if (!TryCopy(Buffer.data(), AsPtr(Ptr), Buffer.size()))
		{
			Throw(MemoryError("read", Ptr));
			return {};
		}
		return ToHex(Buffer.data(), Buffer.size());
	}

	template<typename T>
	static T SafeReadT(uint64_t Ptr)
	{
		T Value{};
		if (!TryCopy(&Value, AsPtr(Ptr), sizeof(T))) Throw(MemoryError("read", Ptr));
		return Value;
	}

	static std::string SafeReadCString(uint64_t Ptr, uint32_t Length)
	{
		std::string Str(Length, '\0');
		if (!TryCopy(Str.data(), AsPtr(Ptr), Length))
		{
			Throw(MemoryError("read", Ptr));
			return {};
		}
		Str.resize(strnlen(Str.data(), Length));
		return Str;
	}

	static void WritePattern(uint64_t Ptr, const std::string& Pattern)
	{
		std::vector<int16_t> Bytes;
		if (!ParsePattern(Pattern, Bytes)) return;

		auto Addr = (uint8_t*)AsPtr(Ptr);
		for (size_t Idx = 0; Idx < Bytes.size(); Idx++)
		{
			if (Bytes[Idx] >= 0) Addr[Idx] = (uint8_t)Bytes[Idx];
		}
	}

	template<typename T>
	static void WriteT(uint64_t Ptr, T Value)
	{
		memcpy(AsPtr(Ptr), &Value, sizeof(T));
	}

	template<typename T>
	static void WriteRefT(uint64_t Ptr, const T& Value)
	{
		memcpy(AsPtr(Ptr), &Value, sizeof(T));
	}

	static void WriteCString(uint64_t Ptr, const std::string& Str)
	{
		memcpy(AsPtr(Ptr), Str.c_str(), Str.size() + 1);
	}

	static void SafeWritePattern(uint64_t Ptr, const std::string& Pattern)
	{
		std::vector<int16_t> Bytes;
		if (!ParsePattern(Pattern, Bytes)) return;

		std::vector<uint8_t> Buffer(Bytes.size());
		if (!TryCopy(Buffer.data(), AsPtr(Ptr), Buffer.size()))
		{
			Throw(MemoryError("read", Ptr));
			return;
		}
		for (size_t Idx = 0; Idx < Bytes.size(); Idx++)
		{
			if (Bytes[Idx] >= 0) Buffer[Idx] = (uint8_t)Bytes[Idx];
		}
		if (!TryCopy(AsPtr(Ptr), Buffer.data(), Buffer.size())) Throw(MemoryError("write", Ptr));
	}

	template<typename T>
	static void SafeWriteT(uint64_t Ptr, T Value)
	{
		if (!TryCopy(AsPtr(Ptr), &Value, sizeof(T))) Throw(MemoryError("write", Ptr));
	}

	template<typename T>
	static void SafeWriteRefT(uint64_t Ptr, const T& Value)
	{
		if (!TryCopy(AsPtr(Ptr), &Value, sizeof(T))) Throw(MemoryError("write", Ptr));
	}

	static void SafeWriteCString(uint64_t Ptr, const std::string& Str)
	{
		if (!TryCopy(AsPtr(Ptr), Str.c_str(), Str.size() + 1)) Throw(MemoryError("write", Ptr));
	}

	template<typename T>
	static T GetOffsetT(void* Ref, int TypeId, uint32_t Offset)
	{
		T Value{};
		if (uint8_t* Nod = NodFrom(Ref, TypeId)) memcpy(&Value, Nod + Offset, sizeof(T));
		return Value;
	}

	static std::string GetOffsetString(void* Ref, int TypeId, uint32_t Offset)
	{
		uint8_t* Nod = NodFrom(Ref, TypeId);
		return Nod ? Gecko::Exports::Trackmania::ReadGameString(*(const CFastString*)(Nod + Offset)) : std::string();
	}

	template<typename T>
	static void SetOffsetT(void* Ref, int TypeId, uint32_t Offset, const T& Value)
	{
		if (uint8_t* Nod = NodFrom(Ref, TypeId)) memcpy(Nod + Offset, &Value, sizeof(T));
	}

	static void SetOffsetNod(void* Ref, int TypeId, uint32_t Offset, CMwNod* Value)
	{
		SetOffsetT<CMwNod*>(Ref, TypeId, Offset, Value);
	}

	static void SetOffsetString(void* Ref, int TypeId, uint32_t Offset, const std::string& Value)
	{
		uint8_t* Nod = NodFrom(Ref, TypeId);
		if (Nod and !Gecko::Exports::Trackmania::AssignGameString((CFastString*)(Nod + Offset), Value)) Throw("Couldn't set the string");
	}

	static bool ForceCastTemplateCallback(asITypeInfo* Type, bool& DontGarbageCollect)
	{
		asITypeInfo* SubType = Type->GetSubType();
		if (!(Type->GetSubTypeId() & asTYPEID_OBJHANDLE) or !SubType or !(SubType->GetFlags() & asOBJ_NOCOUNT))
		{
			Type->GetEngine()->WriteMessage("Dev::ForceCast", 0, 0, asMSGTYPE_ERROR, "ForceCast's subtype must be a handle to a nod");
			return false;
		}
		DontGarbageCollect = true;
		return true;
	}

	static void* CastSource(void* Ref, int TypeId)
	{
		if (!Ref or TypeId == asTYPEID_VOID) return nullptr;

		asIScriptContext* Ctx = asGetActiveContext();
		asITypeInfo* Source = Ctx ? Ctx->GetEngine()->GetTypeInfoById(TypeId) : nullptr;
		if (Source and (Source->GetFlags() & asOBJ_TEMPLATE) and std::string_view(Source->GetName()) == "ForceCast" and std::string_view(Source->GetNamespace()) == "Dev")
			return *(void**)Ref;

		if (!Source or !(Source->GetFlags() & asOBJ_REF))
		{
			Throw("Expected a handle");
			return nullptr;
		}
		return ObjectFrom(Ref, TypeId);
	}

	static void ForceCastConstruct(asITypeInfo*, void* Ref, int TypeId, void** Self)
	{
		*Self = CastSource(Ref, TypeId);
	}

	static void ForceCastCopy(asITypeInfo*, void* const* Other, void** Self)
	{
		*Self = *Other;
	}

	static void ForceCastDestruct(void**)
	{
	}

	static void** ForceCastGet(void** Self)
	{
		return Self;
	}

	static void ForceCastSet(void* Ref, int TypeId, void** Self)
	{
		*Self = CastSource(Ref, TypeId);
	}

	static void** ForceCastAssign(void* const* Other, void** Self)
	{
		*Self = *Other;
		return Self;
	}

	/*
	!CompatMeta
	for     Dev
	with    void Sleep(uint ms)
	with    uint64 BaseAddress()
	with    uint64 BaseAddressEnd()
	with    uint64 FindPattern(const string&in pattern)
	with    string Patch(uint64 ptr, const string&in pattern)
	with    uint64 Allocate(uint size, bool executable = false)
	with    void Free(uint64 ptr)
	with    void DebugBreak()
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("void Sleep(uint ms)", Dev_Sleep);
	DEV_FN("uint64 BaseAddress()", BaseAddress);
	DEV_FN("uint64 BaseAddressEnd()", BaseAddressEnd);
	DEV_FN("uint64 FindPattern(const string&in pattern)", FindPattern);
	DEV_FN("string Patch(uint64 ptr, const string&in pattern)", Patch);
	DEV_FN("uint64 Allocate(uint size, bool executable = false)", Allocate);
	DEV_FN("void Free(uint64 ptr)", Free);
	DEV_FN("void DebugBreak()", Dev_DebugBreak);

	/*
	!CompatMeta
	for     Dev
	with    string Read(uint64 ptr, uint64 size)
	with    int8 ReadInt8(uint64 ptr)
	with    int16 ReadInt16(uint64 ptr)
	with    int ReadInt32(uint64 ptr)
	with    int64 ReadInt64(uint64 ptr)
	with    uint8 ReadUInt8(uint64 ptr)
	with    uint16 ReadUInt16(uint64 ptr)
	with    uint ReadUInt32(uint64 ptr)
	with    uint64 ReadUInt64(uint64 ptr)
	with    uint8 ReadUint8(uint64 ptr)
	with    uint16 ReadUint16(uint64 ptr)
	with    uint ReadUint32(uint64 ptr)
	with    uint64 ReadUint64(uint64 ptr)
	with    float ReadFloat(uint64 ptr)
	with    double ReadDouble(uint64 ptr)
	with    vec2 ReadVec2(uint64 ptr)
	with    vec3 ReadVec3(uint64 ptr)
	with    vec4 ReadVec4(uint64 ptr)
	with    int2 ReadInt2(uint64 ptr)
	with    int3 ReadInt3(uint64 ptr)
	with    nat2 ReadNat2(uint64 ptr)
	with    nat3 ReadNat3(uint64 ptr)
	with    iso3 ReadIso3(uint64 ptr)
	with    iso4 ReadIso4(uint64 ptr)
	with    string ReadCString(uint64 ptr, uint length)
	with    string ReadCString(uint64 ptr)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("string Read(uint64 ptr, uint64 size)", Read);
	DEV_FN("int8 ReadInt8(uint64 ptr)", ReadT<int8_t>);
	DEV_FN("int16 ReadInt16(uint64 ptr)", ReadT<int16_t>);
	DEV_FN("int ReadInt32(uint64 ptr)", ReadT<int32_t>);
	DEV_FN("int64 ReadInt64(uint64 ptr)", ReadT<int64_t>);
	DEV_FN("uint8 ReadUInt8(uint64 ptr)", ReadT<uint8_t>);
	DEV_FN("uint16 ReadUInt16(uint64 ptr)", ReadT<uint16_t>);
	DEV_FN("uint ReadUInt32(uint64 ptr)", ReadT<uint32_t>);
	DEV_FN("uint64 ReadUInt64(uint64 ptr)", ReadT<uint64_t>);
	DEV_FN("uint8 ReadUint8(uint64 ptr)", ReadT<uint8_t>);
	DEV_FN("uint16 ReadUint16(uint64 ptr)", ReadT<uint16_t>);
	DEV_FN("uint ReadUint32(uint64 ptr)", ReadT<uint32_t>);
	DEV_FN("uint64 ReadUint64(uint64 ptr)", ReadT<uint64_t>);
	DEV_FN("float ReadFloat(uint64 ptr)", ReadT<float>);
	DEV_FN("double ReadDouble(uint64 ptr)", ReadT<double>);
	DEV_FN("vec2 ReadVec2(uint64 ptr)", ReadT<vec2>);
	DEV_FN("vec3 ReadVec3(uint64 ptr)", ReadT<vec3>);
	DEV_FN("vec4 ReadVec4(uint64 ptr)", ReadT<vec4>);
	DEV_FN("int2 ReadInt2(uint64 ptr)", ReadT<int2>);
	DEV_FN("int3 ReadInt3(uint64 ptr)", ReadT<int3>);
	DEV_FN("nat2 ReadNat2(uint64 ptr)", ReadT<nat2>);
	DEV_FN("nat3 ReadNat3(uint64 ptr)", ReadT<nat3>);
	DEV_FN("iso3 ReadIso3(uint64 ptr)", ReadT<iso3>);
	DEV_FN("iso4 ReadIso4(uint64 ptr)", ReadT<iso4>);
	DEV_FN("string ReadCString(uint64 ptr, uint length)", ReadCStringLength);
	DEV_FN("string ReadCString(uint64 ptr)", ReadCString);

	/*
	!CompatMeta
	for     Dev
	with    string SafeRead(uint64 ptr, uint64 size)
	with    int8 SafeReadInt8(uint64 ptr)
	with    int16 SafeReadInt16(uint64 ptr)
	with    int SafeReadInt32(uint64 ptr)
	with    int64 SafeReadInt64(uint64 ptr)
	with    uint8 SafeReadUInt8(uint64 ptr)
	with    uint16 SafeReadUInt16(uint64 ptr)
	with    uint SafeReadUInt32(uint64 ptr)
	with    uint64 SafeReadUInt64(uint64 ptr)
	with    uint8 SafeReadUint8(uint64 ptr)
	with    uint16 SafeReadUint16(uint64 ptr)
	with    uint SafeReadUint32(uint64 ptr)
	with    uint64 SafeReadUint64(uint64 ptr)
	with    float SafeReadFloat(uint64 ptr)
	with    double SafeReadDouble(uint64 ptr)
	with    vec2 SafeReadVec2(uint64 ptr)
	with    vec3 SafeReadVec3(uint64 ptr)
	with    vec4 SafeReadVec4(uint64 ptr)
	with    int2 SafeReadInt2(uint64 ptr)
	with    int3 SafeReadInt3(uint64 ptr)
	with    nat2 SafeReadNat2(uint64 ptr)
	with    nat3 SafeReadNat3(uint64 ptr)
	with    iso3 SafeReadIso3(uint64 ptr)
	with    iso4 SafeReadIso4(uint64 ptr)
	with    string SafeReadCString(uint64 ptr, uint length)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("string SafeRead(uint64 ptr, uint64 size)", SafeRead);
	DEV_FN("int8 SafeReadInt8(uint64 ptr)", SafeReadT<int8_t>);
	DEV_FN("int16 SafeReadInt16(uint64 ptr)", SafeReadT<int16_t>);
	DEV_FN("int SafeReadInt32(uint64 ptr)", SafeReadT<int32_t>);
	DEV_FN("int64 SafeReadInt64(uint64 ptr)", SafeReadT<int64_t>);
	DEV_FN("uint8 SafeReadUInt8(uint64 ptr)", SafeReadT<uint8_t>);
	DEV_FN("uint16 SafeReadUInt16(uint64 ptr)", SafeReadT<uint16_t>);
	DEV_FN("uint SafeReadUInt32(uint64 ptr)", SafeReadT<uint32_t>);
	DEV_FN("uint64 SafeReadUInt64(uint64 ptr)", SafeReadT<uint64_t>);
	DEV_FN("uint8 SafeReadUint8(uint64 ptr)", SafeReadT<uint8_t>);
	DEV_FN("uint16 SafeReadUint16(uint64 ptr)", SafeReadT<uint16_t>);
	DEV_FN("uint SafeReadUint32(uint64 ptr)", SafeReadT<uint32_t>);
	DEV_FN("uint64 SafeReadUint64(uint64 ptr)", SafeReadT<uint64_t>);
	DEV_FN("float SafeReadFloat(uint64 ptr)", SafeReadT<float>);
	DEV_FN("double SafeReadDouble(uint64 ptr)", SafeReadT<double>);
	DEV_FN("vec2 SafeReadVec2(uint64 ptr)", SafeReadT<vec2>);
	DEV_FN("vec3 SafeReadVec3(uint64 ptr)", SafeReadT<vec3>);
	DEV_FN("vec4 SafeReadVec4(uint64 ptr)", SafeReadT<vec4>);
	DEV_FN("int2 SafeReadInt2(uint64 ptr)", SafeReadT<int2>);
	DEV_FN("int3 SafeReadInt3(uint64 ptr)", SafeReadT<int3>);
	DEV_FN("nat2 SafeReadNat2(uint64 ptr)", SafeReadT<nat2>);
	DEV_FN("nat3 SafeReadNat3(uint64 ptr)", SafeReadT<nat3>);
	DEV_FN("iso3 SafeReadIso3(uint64 ptr)", SafeReadT<iso3>);
	DEV_FN("iso4 SafeReadIso4(uint64 ptr)", SafeReadT<iso4>);
	DEV_FN("string SafeReadCString(uint64 ptr, uint length)", SafeReadCString);

	/*
	!CompatMeta
	for     Dev
	with    void Write(uint64 ptr, const string&in pattern)
	with    void Write(uint64 ptr, int8 i)
	with    void Write(uint64 ptr, int16 i)
	with    void Write(uint64 ptr, int i)
	with    void Write(uint64 ptr, int64 i)
	with    void Write(uint64 ptr, uint8 i)
	with    void Write(uint64 ptr, uint16 i)
	with    void Write(uint64 ptr, uint i)
	with    void Write(uint64 ptr, uint64 i)
	with    void Write(uint64 ptr, float f)
	with    void Write(uint64 ptr, double f)
	with    void Write(uint64 ptr, const vec2&in v)
	with    void Write(uint64 ptr, const vec3&in v)
	with    void Write(uint64 ptr, const vec4&in v)
	with    void Write(uint64 ptr, const int2&in v)
	with    void Write(uint64 ptr, const int3&in v)
	with    void Write(uint64 ptr, const nat2&in v)
	with    void Write(uint64 ptr, const nat3&in v)
	with    void Write(uint64 ptr, const iso3&in v)
	with    void Write(uint64 ptr, const iso4&in v)
	with    void WriteCString(uint64 ptr, const string&in str)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("void Write(uint64 ptr, const string&in pattern)", WritePattern);
	DEV_FN("void Write(uint64 ptr, int8 i)", WriteT<int8_t>);
	DEV_FN("void Write(uint64 ptr, int16 i)", WriteT<int16_t>);
	DEV_FN("void Write(uint64 ptr, int i)", WriteT<int32_t>);
	DEV_FN("void Write(uint64 ptr, int64 i)", WriteT<int64_t>);
	DEV_FN("void Write(uint64 ptr, uint8 i)", WriteT<uint8_t>);
	DEV_FN("void Write(uint64 ptr, uint16 i)", WriteT<uint16_t>);
	DEV_FN("void Write(uint64 ptr, uint i)", WriteT<uint32_t>);
	DEV_FN("void Write(uint64 ptr, uint64 i)", WriteT<uint64_t>);
	DEV_FN("void Write(uint64 ptr, float f)", WriteT<float>);
	DEV_FN("void Write(uint64 ptr, double f)", WriteT<double>);
	DEV_FN("void Write(uint64 ptr, const vec2&in v)", WriteRefT<vec2>);
	DEV_FN("void Write(uint64 ptr, const vec3&in v)", WriteRefT<vec3>);
	DEV_FN("void Write(uint64 ptr, const vec4&in v)", WriteRefT<vec4>);
	DEV_FN("void Write(uint64 ptr, const int2&in v)", WriteRefT<int2>);
	DEV_FN("void Write(uint64 ptr, const int3&in v)", WriteRefT<int3>);
	DEV_FN("void Write(uint64 ptr, const nat2&in v)", WriteRefT<nat2>);
	DEV_FN("void Write(uint64 ptr, const nat3&in v)", WriteRefT<nat3>);
	DEV_FN("void Write(uint64 ptr, const iso3&in v)", WriteRefT<iso3>);
	DEV_FN("void Write(uint64 ptr, const iso4&in v)", WriteRefT<iso4>);
	DEV_FN("void WriteCString(uint64 ptr, const string&in str)", WriteCString);

	/*
	!CompatMeta
	for     Dev
	with    void SafeWrite(uint64 ptr, const string&in pattern)
	with    void SafeWrite(uint64 ptr, int8 i)
	with    void SafeWrite(uint64 ptr, int16 i)
	with    void SafeWrite(uint64 ptr, int i)
	with    void SafeWrite(uint64 ptr, int64 i)
	with    void SafeWrite(uint64 ptr, uint8 i)
	with    void SafeWrite(uint64 ptr, uint16 i)
	with    void SafeWrite(uint64 ptr, uint i)
	with    void SafeWrite(uint64 ptr, uint64 i)
	with    void SafeWrite(uint64 ptr, float f)
	with    void SafeWrite(uint64 ptr, double f)
	with    void SafeWrite(uint64 ptr, const vec2&in v)
	with    void SafeWrite(uint64 ptr, const vec3&in v)
	with    void SafeWrite(uint64 ptr, const vec4&in v)
	with    void SafeWrite(uint64 ptr, const int2&in v)
	with    void SafeWrite(uint64 ptr, const int3&in v)
	with    void SafeWrite(uint64 ptr, const nat2&in v)
	with    void SafeWrite(uint64 ptr, const nat3&in v)
	with    void SafeWrite(uint64 ptr, const iso3&in v)
	with    void SafeWrite(uint64 ptr, const iso4&in v)
	with    void SafeWriteCString(uint64 ptr, const string&in str)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("void SafeWrite(uint64 ptr, const string&in pattern)", SafeWritePattern);
	DEV_FN("void SafeWrite(uint64 ptr, int8 i)", SafeWriteT<int8_t>);
	DEV_FN("void SafeWrite(uint64 ptr, int16 i)", SafeWriteT<int16_t>);
	DEV_FN("void SafeWrite(uint64 ptr, int i)", SafeWriteT<int32_t>);
	DEV_FN("void SafeWrite(uint64 ptr, int64 i)", SafeWriteT<int64_t>);
	DEV_FN("void SafeWrite(uint64 ptr, uint8 i)", SafeWriteT<uint8_t>);
	DEV_FN("void SafeWrite(uint64 ptr, uint16 i)", SafeWriteT<uint16_t>);
	DEV_FN("void SafeWrite(uint64 ptr, uint i)", SafeWriteT<uint32_t>);
	DEV_FN("void SafeWrite(uint64 ptr, uint64 i)", SafeWriteT<uint64_t>);
	DEV_FN("void SafeWrite(uint64 ptr, float f)", SafeWriteT<float>);
	DEV_FN("void SafeWrite(uint64 ptr, double f)", SafeWriteT<double>);
	DEV_FN("void SafeWrite(uint64 ptr, const vec2&in v)", SafeWriteRefT<vec2>);
	DEV_FN("void SafeWrite(uint64 ptr, const vec3&in v)", SafeWriteRefT<vec3>);
	DEV_FN("void SafeWrite(uint64 ptr, const vec4&in v)", SafeWriteRefT<vec4>);
	DEV_FN("void SafeWrite(uint64 ptr, const int2&in v)", SafeWriteRefT<int2>);
	DEV_FN("void SafeWrite(uint64 ptr, const int3&in v)", SafeWriteRefT<int3>);
	DEV_FN("void SafeWrite(uint64 ptr, const nat2&in v)", SafeWriteRefT<nat2>);
	DEV_FN("void SafeWrite(uint64 ptr, const nat3&in v)", SafeWriteRefT<nat3>);
	DEV_FN("void SafeWrite(uint64 ptr, const iso3&in v)", SafeWriteRefT<iso3>);
	DEV_FN("void SafeWrite(uint64 ptr, const iso4&in v)", SafeWriteRefT<iso4>);
	DEV_FN("void SafeWriteCString(uint64 ptr, const string&in str)", SafeWriteCString);

	/*
	!CompatMeta
	for     Dev
	with    int8 GetOffsetInt8(const ?&in nod, uint offset)
	with    int16 GetOffsetInt16(const ?&in nod, uint offset)
	with    int GetOffsetInt32(const ?&in nod, uint offset)
	with    int64 GetOffsetInt64(const ?&in nod, uint offset)
	with    uint8 GetOffsetUint8(const ?&in nod, uint offset)
	with    uint16 GetOffsetUint16(const ?&in nod, uint offset)
	with    uint GetOffsetUint32(const ?&in nod, uint offset)
	with    uint64 GetOffsetUint64(const ?&in nod, uint offset)
	with    float GetOffsetFloat(const ?&in nod, uint offset)
	with    double GetOffsetDouble(const ?&in nod, uint offset)
	with    vec2 GetOffsetVec2(const ?&in nod, uint offset)
	with    vec3 GetOffsetVec3(const ?&in nod, uint offset)
	with    vec4 GetOffsetVec4(const ?&in nod, uint offset)
	with    int2 GetOffsetInt2(const ?&in nod, uint offset)
	with    int3 GetOffsetInt3(const ?&in nod, uint offset)
	with    nat2 GetOffsetNat2(const ?&in nod, uint offset)
	with    nat3 GetOffsetNat3(const ?&in nod, uint offset)
	with    iso3 GetOffsetIso3(const ?&in nod, uint offset)
	with    iso4 GetOffsetIso4(const ?&in nod, uint offset)
	with    CMwNod@ GetOffsetNod(const ?&in nod, uint offset)
	with    string GetOffsetString(const ?&in nod, uint offset)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("int8 GetOffsetInt8(const ?&in nod, uint offset)", GetOffsetT<int8_t>);
	DEV_FN("int16 GetOffsetInt16(const ?&in nod, uint offset)", GetOffsetT<int16_t>);
	DEV_FN("int GetOffsetInt32(const ?&in nod, uint offset)", GetOffsetT<int32_t>);
	DEV_FN("int64 GetOffsetInt64(const ?&in nod, uint offset)", GetOffsetT<int64_t>);
	DEV_FN("uint8 GetOffsetUint8(const ?&in nod, uint offset)", GetOffsetT<uint8_t>);
	DEV_FN("uint16 GetOffsetUint16(const ?&in nod, uint offset)", GetOffsetT<uint16_t>);
	DEV_FN("uint GetOffsetUint32(const ?&in nod, uint offset)", GetOffsetT<uint32_t>);
	DEV_FN("uint64 GetOffsetUint64(const ?&in nod, uint offset)", GetOffsetT<uint64_t>);
	DEV_FN("float GetOffsetFloat(const ?&in nod, uint offset)", GetOffsetT<float>);
	DEV_FN("double GetOffsetDouble(const ?&in nod, uint offset)", GetOffsetT<double>);
	DEV_FN("vec2 GetOffsetVec2(const ?&in nod, uint offset)", GetOffsetT<vec2>);
	DEV_FN("vec3 GetOffsetVec3(const ?&in nod, uint offset)", GetOffsetT<vec3>);
	DEV_FN("vec4 GetOffsetVec4(const ?&in nod, uint offset)", GetOffsetT<vec4>);
	DEV_FN("int2 GetOffsetInt2(const ?&in nod, uint offset)", GetOffsetT<int2>);
	DEV_FN("int3 GetOffsetInt3(const ?&in nod, uint offset)", GetOffsetT<int3>);
	DEV_FN("nat2 GetOffsetNat2(const ?&in nod, uint offset)", GetOffsetT<nat2>);
	DEV_FN("nat3 GetOffsetNat3(const ?&in nod, uint offset)", GetOffsetT<nat3>);
	DEV_FN("iso3 GetOffsetIso3(const ?&in nod, uint offset)", GetOffsetT<iso3>);
	DEV_FN("iso4 GetOffsetIso4(const ?&in nod, uint offset)", GetOffsetT<iso4>);
	DEV_FN("CMwNod@ GetOffsetNod(const ?&in nod, uint offset)", GetOffsetT<CMwNod*>);
	DEV_FN("string GetOffsetString(const ?&in nod, uint offset)", GetOffsetString);

	/*
	!CompatMeta
	for     Dev
	with    void SetOffset(const ?&in nod, uint offset, const int8&in v)
	with    void SetOffset(const ?&in nod, uint offset, const int16&in v)
	with    void SetOffset(const ?&in nod, uint offset, const int&in v)
	with    void SetOffset(const ?&in nod, uint offset, const int64&in v)
	with    void SetOffset(const ?&in nod, uint offset, const uint8&in v)
	with    void SetOffset(const ?&in nod, uint offset, const uint16&in v)
	with    void SetOffset(const ?&in nod, uint offset, const uint&in v)
	with    void SetOffset(const ?&in nod, uint offset, const uint64&in v)
	with    void SetOffset(const ?&in nod, uint offset, const float&in v)
	with    void SetOffset(const ?&in nod, uint offset, const double&in v)
	with    void SetOffset(const ?&in nod, uint offset, const vec2&in v)
	with    void SetOffset(const ?&in nod, uint offset, const vec3&in v)
	with    void SetOffset(const ?&in nod, uint offset, const vec4&in v)
	with    void SetOffset(const ?&in nod, uint offset, const int2&in v)
	with    void SetOffset(const ?&in nod, uint offset, const int3&in v)
	with    void SetOffset(const ?&in nod, uint offset, const nat2&in v)
	with    void SetOffset(const ?&in nod, uint offset, const nat3&in v)
	with    void SetOffset(const ?&in nod, uint offset, const iso3&in v)
	with    void SetOffset(const ?&in nod, uint offset, const iso4&in v)
	with    void SetOffset(const ?&in nod, uint offset, CMwNod@ newNod)
	with    void SetOffset(const ?&in nod, uint offset, const string&in str)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int8&in v)", SetOffsetT<int8_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int16&in v)", SetOffsetT<int16_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int&in v)", SetOffsetT<int32_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int64&in v)", SetOffsetT<int64_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const uint8&in v)", SetOffsetT<uint8_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const uint16&in v)", SetOffsetT<uint16_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const uint&in v)", SetOffsetT<uint32_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const uint64&in v)", SetOffsetT<uint64_t>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const float&in v)", SetOffsetT<float>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const double&in v)", SetOffsetT<double>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const vec2&in v)", SetOffsetT<vec2>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const vec3&in v)", SetOffsetT<vec3>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const vec4&in v)", SetOffsetT<vec4>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int2&in v)", SetOffsetT<int2>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const int3&in v)", SetOffsetT<int3>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const nat2&in v)", SetOffsetT<nat2>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const nat3&in v)", SetOffsetT<nat3>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const iso3&in v)", SetOffsetT<iso3>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const iso4&in v)", SetOffsetT<iso4>);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, CMwNod@ newNod)", SetOffsetNod);
	DEV_FN("void SetOffset(const ?&in nod, uint offset, const string&in str)", SetOffsetString);

	/*
	!CompatMeta
	for     Dev
	with    ForceCast(int&in, const ?&in)
	with    ~ForceCast()
	being   behavior
	of      ForceCast
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GECKO_EXPORT_BEHAVIOUR(s_Exports, "Dev", "ForceCast<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ForceCastTemplateCallback), asCALL_CDECL);
	FORCECAST_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(int&in, const ?&in)", ForceCastConstruct);
	FORCECAST_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(int&in, const ForceCast<T>&in)", ForceCastCopy);
	FORCECAST_BEHAVIOUR(asBEHAVE_DESTRUCT, "void f()", ForceCastDestruct);

	/*
	!CompatMeta
	for     Dev
	with    T& Get()
	with    void Set(const ?&in)
	being   method
	of      ForceCast
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	FORCECAST_METHOD("T& Get()", ForceCastGet);
	FORCECAST_METHOD("void Set(const ?&in)", ForceCastSet);
	FORCECAST_METHOD("ForceCast<T>& opAssign(const ForceCast<T>&in)", ForceCastAssign);

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("Dev");
		Engine->RegisterObjectType("ForceCast<class T>", sizeof(void*), asOBJ_VALUE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_CD);
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
