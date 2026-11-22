#include "pch.h"
#include "Trackmania.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Twinkie/Twinkie.h>
#include <unordered_set>

#undef GetObject
#undef RegisterClass

namespace Gecko::Exports::Trackmania
{
	std::unordered_map<uint32_t, CMwClassInfo*> ClassIDToInfo;
	std::unordered_map<std::string, CMwClassInfo*> ClassNameToInfo;
#ifdef TMCN
	std::unordered_map<uint32_t, CMwEnumInfo*> EnumIDToInfo;
	std::unordered_map<std::string, CMwEnumInfo*> EnumNameToInfo;
#endif

	static std::vector<GeckoExport> s_BaseExports;
	static std::vector<GeckoExport> s_Exports;

#define FASTARRAY_METHOD(Decl_, Ptr_, Conv_)          GECKO_EXPORT_METHOD(s_BaseExports, "", "MwFastArray<T>", Decl_, Ptr_, Conv_)
#define FASTARRAY_BEHAVIOUR(Beh_, Decl_, Ptr_, Conv_) GECKO_EXPORT_BEHAVIOUR(s_BaseExports, "", "MwFastArray<T>", Beh_, Decl_, Ptr_, Conv_)
#define FASTBUFFER_METHOD(Decl_, Ptr_, Conv_)          GECKO_EXPORT_METHOD(s_BaseExports, "", "MwFastBuffer<T>", Decl_, Ptr_, Conv_)
#define FASTBUFFER_BEHAVIOUR(Beh_, Decl_, Ptr_, Conv_) GECKO_EXPORT_BEHAVIOUR(s_BaseExports, "", "MwFastBuffer<T>", Beh_, Decl_, Ptr_, Conv_)

	static bool AllowStringSubtypes = false;

	namespace BaseTypes
	{
		//static void __cdecl CMwNod_AddRef(CMwNod* Nod)
		//{
		//	Nod->ReferenceCount++;
		//}

		//static void __cdecl CMwNod_RemoveRef(CMwNod* Nod)
		//{
		//	Nod->ReferenceCount--;
		//}

		/*
		!CompatMeta
		for     Global
		with    uint get_Length() const
		as      uint get_Length()
		being   method
		of      MwFastArray
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		static uint32_t __cdecl CFastArray_Length(CFastArrayGen* Array)
		{
			return Array->Size;
		}
		FASTARRAY_METHOD("const uint get_Length() property", asFUNCTION(CFastArray_Length), asCALL_CDECL_OBJLAST);

		// Size in bytes of one element of a MwFastArray<T> instance
		static uint32_t CFastArray_ElemSize(asIScriptEngine* Engine, asITypeInfo* ArrayType)
		{
			int SubTypeId = ArrayType->GetSubTypeId();

			if (SubTypeId == asTYPEID_BOOL)
				return sizeof(uint32_t);

			if ((SubTypeId & asTYPEID_MASK_OBJECT) == 0)
				return Engine->GetSizeOfPrimitiveType(SubTypeId);

			if (SubTypeId & asTYPEID_OBJHANDLE)
				return sizeof(void*);

			auto SubType = Engine->GetTypeInfoById(SubTypeId);
			if (SubType->GetFlags() & asOBJ_VALUE)
				return SubType->GetSize();

			return sizeof(void*);
		}

		static void __cdecl CFastArray_opIndex(asIScriptGeneric* Script)
		{
			auto Engine = Script->GetEngine();
			uint32_t ElemSize = CFastArray_ElemSize(Engine, Engine->GetTypeInfoById(Script->GetObjectTypeId()));

			uint32_t Idx = Script->GetArgDWord(0);
			CFastArrayGen* Array = (CFastArrayGen*)Script->GetObject();

			if (Idx >= Array->Size || !Array->Ptr)
			{
				if (auto Ctx = asGetActiveContext()) Ctx->SetException("Index out of bounds");
				return;
			}

			Script->SetReturnAddress(Array->Get(Idx, ElemSize));
		}
		FASTARRAY_METHOD("T& opIndex(uint index)", asFUNCTION(CFastArray_opIndex), asCALL_GENERIC);
		FASTARRAY_METHOD("const T& opIndex(uint index) const", asFUNCTION(CFastArray_opIndex), asCALL_GENERIC);

		static uint32_t __cdecl CFastBuffer_Length(CFastBufferGen* Buffer)
		{
			return Buffer->Size;
		}
		FASTBUFFER_METHOD("const uint get_Length() property", asFUNCTION(CFastBuffer_Length), asCALL_CDECL_OBJLAST);

		static void __cdecl CFastBuffer_opIndex(asIScriptGeneric* Script)
		{
			auto Engine = Script->GetEngine();
			uint32_t ElemSize = CFastArray_ElemSize(Engine, Engine->GetTypeInfoById(Script->GetObjectTypeId()));

			uint32_t Idx = Script->GetArgDWord(0);
			CFastBufferGen* Buffer = (CFastBufferGen*)Script->GetObject();

			if (Idx >= Buffer->Size || !Buffer->Ptr)
			{
				if (auto Ctx = asGetActiveContext()) Ctx->SetException("Index out of bounds");
				return;
			}

			Script->SetReturnAddress(Buffer->Get(Idx, ElemSize));
		}
		FASTBUFFER_METHOD("T& opIndex(uint index)", asFUNCTION(CFastBuffer_opIndex), asCALL_GENERIC);
		FASTBUFFER_METHOD("const T& opIndex(uint index) const", asFUNCTION(CFastBuffer_opIndex), asCALL_GENERIC);

		static bool __cdecl CFastArray_TemplateCallback(asITypeInfo* ArrayType, bool& DontGarbageCollect)
		{
			DontGarbageCollect = true;

			int SubTypeId = ArrayType->GetSubTypeId();
			if (SubTypeId == asTYPEID_VOID) return false;

			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
			{
				auto SubType = ArrayType->GetEngine()->GetTypeInfoById(SubTypeId);
				if ((SubType->GetFlags() & asOBJ_VALUE) && !(SubType->GetFlags() & asOBJ_POD))
				{
					std::string_view Name = SubType->GetName();
					if (AllowStringSubtypes && (Name == "string" || Name == "wstring")) return true;

					// TODO: Raise an error here
					return false;
				}
			}

			return true;
		}
		FASTARRAY_BEHAVIOUR(asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(CFastArray_TemplateCallback), asCALL_CDECL);
		FASTBUFFER_BEHAVIOUR(asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(CFastArray_TemplateCallback), asCALL_CDECL);

		// All arrays made by a script
		static std::unordered_set<CFastArrayGen*> OwnedArrays;

		static void CFastArray_FreeBuffer(asIScriptEngine* Engine, asITypeInfo* ArrayType, CFastArrayGen* Array)
		{
			if (not Array->Ptr) return;

			int SubTypeId = ArrayType->GetSubTypeId();
			if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				auto SubType = Engine->GetTypeInfoById(SubTypeId);
				auto Elems = (void**)Array->Ptr;
				for (uint32_t i = 0; i < Array->Size; i++)
					if (Elems[i]) Engine->ReleaseScriptObject(Elems[i], SubType);
			}

			delete[] (uint8_t*)Array->Ptr;
			Array->Ptr = nullptr;
			Array->Size = 0;
		}

		static void CFastArray_Construct(asITypeInfo* ArrayType, uint32_t Size, void* Memory)
		{
			CFastArrayGen* NewArray = new (Memory) CFastArrayGen();
			OwnedArrays.insert(NewArray);

#ifdef GAMEBOX
			// Also copy the vftable because yes, arrays have vftables now
			// Evil hack incoming
			static uintptr_t CachedVf = 0;
			if (!CachedVf)
			{
				auto& Twinkie = gTwinkie.TrackmaniaMgr;
				auto Resources = Twinkie.ParamGet<CMwNod*>(Twinkie.GetApp(), "Resources");
				auto AudioSources = Resources ? Twinkie.ParamGet<CFastArrayGen>(Resources, "AudioSources") : nullptr;
				if (AudioSources) CachedVf = AudioSources->vf;
			}

			// TODO: Something better ^^
			NewArray->vf = CachedVf;
#endif
			if (Size == 0) return;

			uint32_t ElemSize = CFastArray_ElemSize(ArrayType->GetEngine(), ArrayType);

			NewArray->Ptr = new uint8_t[(size_t)Size * ElemSize]();
			NewArray->Size = Size;
		}


		static void CFastArray_ctor(asIScriptGeneric* Script)
		{
			asITypeInfo* ArrayType = *(asITypeInfo**)Script->GetAddressOfArg(0);
			CFastArray_Construct(ArrayType, 0, Script->GetObject());
		}
		FASTARRAY_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(CFastArray_ctor), asCALL_GENERIC);

		/*
		!CompatMeta
		for     Global
		with    MwFastArray(int&in)
		as      MwFastArray(int&in)
		being   behavior
		of      MwFastArray
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		static void CFastArray_ctorSize(asIScriptGeneric* Script)
		{
			asITypeInfo* ArrayType = *(asITypeInfo**)Script->GetAddressOfArg(0);
			CFastArray_Construct(ArrayType, Script->GetArgDWord(1), Script->GetObject());
		}
		FASTARRAY_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(int&in, uint)", asFUNCTION(CFastArray_ctorSize), asCALL_GENERIC);

		/*
		!CompatMeta
		for     Global
		with    ~MwFastArray()
		as      ~MwFastArray()
		being   behavior
		of      MwFastArray
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		static void CFastArray_dtor(asIScriptGeneric* Script)
		{
			auto Engine = Script->GetEngine();
			auto Array = (CFastArrayGen*)Script->GetObject();

			if (!OwnedArrays.erase(Array)) return;

			CFastArray_FreeBuffer(Engine, Engine->GetTypeInfoById(Script->GetObjectTypeId()), Array);
		}
		FASTARRAY_BEHAVIOUR(asBEHAVE_DESTRUCT, "void f()", asFUNCTION(CFastArray_dtor), asCALL_GENERIC);

		/*
		!CompatMeta
		for     Global
		with    MwFastArray<T>& opAssign(const MwFastArray<T>&in)
		as      MwFastArray<T>& opAssign(const MwFastArray<T>&in)
		being   method
		of      MwFastArray
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		static void CFastArray_opAssign(asIScriptGeneric* Script)
		{
			auto Engine = Script->GetEngine();
			auto Dst = (CFastArrayGen*)Script->GetObject();
			auto Src = (CFastArrayGen*)Script->GetArgObject(0);

			Script->SetReturnAddress(Dst);

			if (Dst == Src) return;

			if (!OwnedArrays.contains(Dst))
			{
				if (auto Ctx = asGetActiveContext()) (void)0; // TODO: Raise an error, cannot assign to nod's array
				return;
			}

			auto ArrayType = Engine->GetTypeInfoById(Script->GetObjectTypeId());
			int SubTypeId = ArrayType->GetSubTypeId();
			uint32_t ElemSize = CFastArray_ElemSize(Engine, ArrayType);

			uint8_t* NewPtr = nullptr;
			uint32_t NewSize = (Src->Ptr ? Src->Size : 0);
			if (NewSize)
			{
				NewPtr = new uint8_t[(size_t)NewSize * ElemSize];
				memcpy(NewPtr, Src->Ptr, (size_t)NewSize * ElemSize);

				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					auto SubType = Engine->GetTypeInfoById(SubTypeId);
					auto Elems = (void**)NewPtr;
					for (uint32_t i = 0; i < NewSize; i++)
						if (Elems[i]) Engine->AddRefScriptObject(Elems[i], SubType);
				}
			}

			CFastArray_FreeBuffer(Engine, ArrayType, Dst);

			Dst->Ptr = NewPtr;
			Dst->Size = NewSize;
		}
		FASTARRAY_METHOD("MwFastArray<T>& opAssign(const MwFastArray<T>&in)", asFUNCTION(CFastArray_opAssign), asCALL_GENERIC);

		// Registers all base types used by the game.
		static void RegistrarBaseTypes(asIScriptEngine* Engine)
		{
			Engine->RegisterObjectType("MwFastArray<T>", sizeof(CFastArrayGen), asOBJ_VALUE | asOBJ_TEMPLATE | asGetTypeTraits<CFastArrayGen>());
			Engine->RegisterObjectType("MwFastBuffer<T>", 0, asOBJ_REF | asOBJ_NOCOUNT | asOBJ_TEMPLATE);
			RegisterAll(Engine, s_BaseExports);
		}
	}

	// CGameApp@ GetApp()
	static CMwNod* __stdcall GetApp()
	{
		CMwNod* App = ReadAddr(CMwNod*, (uintptr_t)GetModuleHandleA(NULL) + O_APP);
		return App;
	}
#ifdef TMCN
	GECKO_EXPORT_FN_EX(s_Exports, "", "CGameCtnApp@ GetApp()", asFUNCTION(GetApp), asCALL_STDCALL);
#else
	GECKO_EXPORT_FN_EX(s_Exports, "", "CGameApp@ GetApp()", asFUNCTION(GetApp), asCALL_STDCALL);
#endif
	void Caster(asIScriptGeneric* Script)
	{
		CMwNod* FromNod = (CMwNod*)Script->GetObject();

		CMwClassInfo* To = (CMwClassInfo*)Script->GetAuxiliary();
		CMwClassInfo* From = FromNod->MwGetClassInfo();

		while (From)
		{
			if (From->ClassID == To->ClassID)
			{
				Script->SetReturnAddress(FromNod);
				return;
			}
			From = From->ParentClassInfo;
		}
		Script->SetReturnAddress(nullptr);
	}

	void VirtualGetAs(asIScriptGeneric* Script)
	{
		using enum CMwMemberInfo::EType;

		CMwNod* Nod = (CMwNod*)Script->GetObject();
		CMwMemberInfo* Member = (CMwMemberInfo*)Script->GetAuxiliary();

		// TODO: Add other types.
		switch (Member->MemberType)
		{
		case CLASS:
			Script->SetReturnAddress(gTwinkie.TrackmaniaMgr.ParamGet<CMwNod*>(Nod, Member));
			break;
		case NATURAL:
			{
				auto pV = gTwinkie.TrackmaniaMgr.ParamGet<uint32_t>(Nod, Member);
				Script->SetReturnDWord(pV ? 0 : *pV);
				break;
			}
		default:
			break;
		}

		if (Member->MemberType == CMwMemberInfo::CLASS)
		{
			Script->SetReturnAddress(gTwinkie.TrackmaniaMgr.ParamGet<CMwNod*>(Nod, Member));
		}
	}

	void VirtualSetAs(asIScriptGeneric* Script)
	{
		using enum CMwMemberInfo::EType;

		CMwNod* Nod = (CMwNod*)Script->GetObject();
		CMwMemberInfo* Member = (CMwMemberInfo*)Script->GetAuxiliary();

		uint32_t Bool = 0;
		void* Value = nullptr;
		switch (Member->MemberType)
		{
		case CLASS:
			Value = Script->GetArgObject(0);
			break;
		case BOOL:
			Bool = *(bool*)Script->GetAddressOfArg(0) ? 1 : 0;
			Value = &Bool;
			break;
		default:
			Value = (Script->GetArgTypeId(0) & asTYPEID_MASK_OBJECT) ? Script->GetArgObject(0) : Script->GetAddressOfArg(0);
			break;
		}

		CMwStack Stack{};
		gTwinkie.TrackmaniaMgr.PushToStack(&Stack, Member);
		Nod->VirtualParam_Set(&Stack, Value);
	}

	static std::string VirtualSetType(CMwMemberInfo* Member)
	{
		using enum CMwMemberInfo::EType;

		switch (Member->MemberType)
		{
		case CLASS: return std::format("{}@", ((CMwMemberInfoClass*)Member)->ClassInfo->GetName());
		case BOOL: return "bool";
		case INT:
		case INTRANGE: return "int";
		case NATURAL:
		case NATURALRANGE: return "uint";
		case REAL:
		case REALRANGE: return "float";
		case COLOR:
		case VEC3: return "vec3";
		case VEC2: return "vec2";
		case VEC4: return "vec4";
		case ISO3: return "iso3";
		case ISO4: return "iso4";
#ifdef TMCN
		case INT2: return "int2";
		case INT3: return "int3";
		case NAT2: return "nat2";
		case NAT3: return "nat3";
#endif
		default: return {};
		}
	}

	static void RegisterVirtualSetter(asIScriptEngine* Engine, CMwClassInfo* Class, CMwMemberInfo* Member)
	{
		std::string Type = VirtualSetType(Member);
		if (Type.empty()) return;

		auto asClass = Engine->GetTypeInfoByName(Class->GetName().c_str());
		if (!asClass) return;

		std::string Decl = std::format("void set_{}({} value) property", Member->GetName(), Type);
		if (asClass->GetMethodByDecl(Decl.c_str())) return;

		Engine->RegisterObjectMethod(Class->GetName().c_str(), Decl.c_str(), asFUNCTION(VirtualSetAs), asCALL_GENERIC, Member);
	}

	void CasterNonPersistent(asIScriptGeneric* Script)
	{
		Script->SetReturnAddress(nullptr);
	}

	namespace GameStrings
	{
		template<typename StrT> struct Traits;
		template<> struct Traits<CFastString> { using Script = std::string; static constexpr const char* Name = "string"; };
#ifndef TM1
		template<> struct Traits<CFastStringInt> { using Script = std::wstring; static constexpr const char* Name = "wstring"; };
#endif
		template<typename StrT> using ScriptT = typename Traits<StrT>::Script;


		static std::string Read(const CFastString& Str)
		{
			if (!Str.CStr) return {};
#ifdef TM1
			return std::string(Str.CStr, strnlen(Str.CStr, Str.Count));
#else
			return std::string(Str.CStr, Str.Count);
#endif
		}

#ifndef TM1
		static std::wstring Read(const CFastStringInt& Str)
		{
			if (!Str.CStr) return {};
			return std::wstring(Str.CStr, Str.Count);
		}
#endif

		template<typename StrT>
		static uintptr_t SetLengthFn()
		{
			constexpr bool Wide = not std::is_same_v<StrT, CFastString>;
#if defined(TMCN)
			static const std::vector<const char*> NarrowSigs = { "48 89 5C 24 08 57 48 83 EC 20 48 8B F9 8B DA 3B 51 08 74 ?? 45 33 C9 8B D3 45 8D 41 01 E8 ?? ?? ?? ?? 48 8B 07 C6 04 03 00 89 5F 08" };
			static const std::vector<const char*> WideSigs = { "48 89 5C 24 08 57 48 83 EC 20 48 8B F9 8B DA 3B 51 08 74 ?? 45 33 C9 8B D3 45 8D 41 01 E8 ?? ?? ?? ?? 48 8B 07 33 C9 66 89 0C 58 89 5F 08" };
#elif defined(TM1)
			static const std::vector<const char*> NarrowSigs = { "8B 44 24 04 53 55 8B E9 8D 58 01 3B 5D 04 56 57 74 ?? 8B 4D 08 51 E8 ?? ?? ?? ?? 33 C0 83 C4 04 3B D8 89 45 08 89 45 04 74 ?? 53 89 5D 04 E8 ?? ?? ?? ?? 83 C4 04 89 45 08" };
			static const std::vector<const char*> WideSigs;
#else
			static const std::vector<const char*> NarrowSigs = {
				"53 56 8B 74 24 0C 57 8B F9 8B 1F 3B F3 74 ?? 6A 00 6A 01 56 E8 ?? ?? ?? ?? 8B 47 04 C6 04 06 00 89 37",
				"53 56 8B F1 8B ?? 57 8B 7C 24 10 3B FB 74 ?? 6A 00 6A 01 57 E8 ?? ?? ?? ?? 8B 46 04 C6 04 07 00 89 3E",
			};
			static const std::vector<const char*> WideSigs = { "53 56 8B 74 24 0C 57 8B F9 8B 1F 3B F3 74 ?? 6A 00 6A 01 56 E8 ?? ?? ?? ?? 8B 47 04 66 C7 04 70 00 00 89 37" };
#endif
			static const uintptr_t Fn = []
			{
				for (const char* Sig : Wide ? WideSigs : NarrowSigs)
				{
					if (uintptr_t Found = gTwinkie.TrackmaniaMgr.SigScan(Sig)) return Found;
				}
				return (uintptr_t)0;
			}();
			return Fn;
		}

		template<typename StrT>
		static bool Assign(StrT* Str, const ScriptT<StrT>& Value)
		{
			using CharT = typename ScriptT<StrT>::value_type;
			uintptr_t Fn = SetLengthFn<StrT>();
			if (!Fn) return false;
#ifdef TM1
			((void(__thiscall*)(StrT*, uint32_t, const CharT*))Fn)(Str, (uint32_t)Value.size(), Value.data());
#else
			if (!Str->CStr) return false;
#ifdef GAMEBOX
			((void(__thiscall*)(StrT*, uint32_t, int, int))Fn)(Str, (uint32_t)Value.size(), 0, 0);
#else
			((void(*)(StrT*, uint32_t))Fn)(Str, (uint32_t)Value.size());
#endif
			memcpy(Str->CStr, Value.data(), Value.size() * sizeof(CharT));
#endif
			return true;
		}

		template<typename StrT>
		static ScriptT<StrT> Get(CMwNod* Nod, CMwMemberInfo* Member)
		{
			if (!TwinkTrackmania::IsVirtualGet(Member)) return Read(ReadAddr(StrT, (uintptr_t)Nod + Member->MemberOffset));

			struct
			{
				StrT* pReturnValue = nullptr;
				StrT Storage{};
			} Result;

			CMwStack Stack{};
			gTwinkie.TrackmaniaMgr.PushToStack(&Stack, Member);
			Nod->VirtualParam_Get(&Stack, &Result);

			return Result.pReturnValue ? Read(*Result.pReturnValue) : ScriptT<StrT>();
		}

		template<typename StrT>
		static bool VirtualSet(CMwNod* Nod, CMwMemberInfo* Member, const ScriptT<StrT>& Value)
		{
			ScriptT<StrT> Copy = Value;
			StrT Str{};
			Str.CStr = Copy.data();
#ifdef TM1
			Str.Count = (uint32_t)Copy.size() + 1;
#else
			Str.Count = (uint32_t)Copy.size();
#endif
			CMwStack Stack{};
			gTwinkie.TrackmaniaMgr.PushToStack(&Stack, Member);
			return !Nod->VirtualParam_Set(&Stack, &Str);
		}

		template<typename StrT>
		static bool Set(CMwNod* Nod, CMwMemberInfo* Member, const ScriptT<StrT>& Value)
		{
			if (TwinkTrackmania::IsVirtualSet(Member) || !SetLengthFn<StrT>()) return VirtualSet<StrT>(Nod, Member, Value);
			return Assign((StrT*)((uintptr_t)Nod + Member->MemberOffset), Value);
		}

		template<typename StrT>
		static void Getter(asIScriptGeneric* Gen)
		{
			auto Nod = (CMwNod*)Gen->GetObject();
			auto Member = (CMwMemberInfo*)Gen->GetAuxiliary();
			new (Gen->GetAddressOfReturnLocation()) ScriptT<StrT>(Get<StrT>(Nod, Member));
		}

		template<typename StrT>
		static void Setter(asIScriptGeneric* Gen)
		{
			auto Nod = (CMwNod*)Gen->GetObject();
			auto Member = (CMwMemberInfo*)Gen->GetAuxiliary();
			Set<StrT>(Nod, Member, *(const ScriptT<StrT>*)Gen->GetArgObject(0));
		}

		template<typename StrT>
		static void Register(asIScriptEngine* Engine, CMwClassInfo* Class, CMwMemberInfo* Member)
		{
			auto Type = Engine->GetTypeInfoByName(Class->GetName().c_str());
			if (!Type) return;

			std::string GetDecl = std::format("{} get_{}() property", Traits<StrT>::Name, Member->GetName());
			if (!Type->GetMethodByDecl(GetDecl.c_str()))
			{
				Engine->RegisterObjectMethod(Class->GetName().c_str(), GetDecl.c_str(), asFUNCTION(Getter<StrT>), asCALL_GENERIC, Member);
			}

			std::string SetDecl = std::format("void set_{}(const {}&in) property", Member->GetName(), Traits<StrT>::Name);
			if (!Type->GetMethodByDecl(SetDecl.c_str()))
			{
				Engine->RegisterObjectMethod(Class->GetName().c_str(), SetDecl.c_str(), asFUNCTION(Setter<StrT>), asCALL_GENERIC, Member);
			}
		}

		static void Register(asIScriptEngine* Engine, CMwClassInfo* Class, CMwMemberInfo* Member)
		{
#ifndef TM1
			if (Member->MemberType == CMwMemberInfo::STRINGINT) Register<CFastStringInt>(Engine, Class, Member);
			else
#endif
			Register<CFastString>(Engine, Class, Member);
		}

		template<typename ArrayT, typename StrT>
		static StrT* Element(asIScriptGeneric* Gen)
		{
			auto Array = (ArrayT*)Gen->GetObject();
			uint32_t Idx = Gen->GetArgDWord(0);
			return Array->Ptr && Idx < Array->Size ? (StrT*)Array->Ptr + Idx : nullptr;
		}

		template<typename ArrayT, typename StrT>
		static void ArrayGet(asIScriptGeneric* Gen)
		{
			StrT* Str = Element<ArrayT, StrT>(Gen);
			new (Gen->GetAddressOfReturnLocation()) ScriptT<StrT>(Str ? Read(*Str) : ScriptT<StrT>());
		}

		template<typename ArrayT, typename StrT>
		static void ArraySet(asIScriptGeneric* Gen)
		{
			if (StrT* Str = Element<ArrayT, StrT>(Gen)) Assign(Str, *(const ScriptT<StrT>*)Gen->GetArgObject(1));
		}

		template<typename ArrayT>
		static void ArrayLength(asIScriptGeneric* Gen)
		{
			Gen->SetReturnDWord(((ArrayT*)Gen->GetObject())->Size);
		}

		static void ArrayConstruct(asIScriptGeneric* Gen) { new (Gen->GetObject()) CFastArrayGen(); }
		static void ArrayDestruct(asIScriptGeneric*) {}

		template<typename ArrayT, typename StrT>
		static void RegisterArrayMethods(asIScriptEngine* Engine, const std::string& Name)
		{
			Engine->RegisterObjectMethod(Name.c_str(), std::format("{} get_opIndex(uint index) const property", Traits<StrT>::Name).c_str(), asFUNCTION((ArrayGet<ArrayT, StrT>)), asCALL_GENERIC);
			Engine->RegisterObjectMethod(Name.c_str(), std::format("void set_opIndex(uint index, const {}&in) property", Traits<StrT>::Name).c_str(), asFUNCTION((ArraySet<ArrayT, StrT>)), asCALL_GENERIC);
			Engine->RegisterObjectMethod(Name.c_str(), "const uint get_Length() property", asFUNCTION(ArrayLength<ArrayT>), asCALL_GENERIC);
		}

		template<typename StrT>
		static void RegisterArrays(asIScriptEngine* Engine)
		{
			std::string ArrayName = std::format("MwFastArray<{}>", Traits<StrT>::Name);
			Engine->RegisterObjectType(ArrayName.c_str(), sizeof(CFastArrayGen), asOBJ_VALUE | asOBJ_APP_CLASS_CD);
			Engine->RegisterObjectBehaviour(ArrayName.c_str(), asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ArrayConstruct), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour(ArrayName.c_str(), asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ArrayDestruct), asCALL_GENERIC);
			RegisterArrayMethods<CFastArrayGen, StrT>(Engine, ArrayName);

			std::string BufferName = std::format("MwFastBuffer<{}>", Traits<StrT>::Name);
			Engine->RegisterObjectType(BufferName.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT);
			RegisterArrayMethods<CFastBufferGen, StrT>(Engine, BufferName);
		}
	}

	static inline void RegisterClass(asIScriptEngine* Engine, CMwClassInfo* Class)
	{
		if (Engine->GetTypeInfoByName(Class->GetName().c_str())) return;
		ClassIDToInfo[Class->ClassID] = Class;
		ClassNameToInfo[Class->GetName()] = Class;
		Engine->RegisterObjectType(Class->GetName().c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT);

		if (Class->CtorFn) Engine->RegisterObjectBehaviour(Class->GetName().c_str(), asBEHAVE_FACTORY, (Class->GetName() + std::string("@ f()")).c_str(), (uintptr_t)Class->CtorFn, asCALL_CDECL);
	
//		// TODO: Check if this is how enum registration should be
//#ifdef TMCN
//		if (Class->ClassEnums)
//		{
//			uint32_t EnumID = Class->ClassID;
//			for (size_t EnumIdx = 0; EnumIdx < Class->ClassEnumsCount; EnumIdx++)
//			{
//				CMwEnumInfo* Enum = Class->ClassEnums[EnumIdx];
//
//				Engine->RegisterEnum(Enum->EnumName);
//
//				size_t EnumValueIdx = 0;
//				for (auto& EnumValue : *Enum)
//				{
//					Engine->RegisterEnumValue(Enum->EnumName, EnumValue, (int)EnumValueIdx);
//					EnumValueIdx++;
//				}
//
//				EnumIDToInfo[EnumID] = Enum;
//				EnumNameToInfo[Enum->EnumName] = Enum;
//
//				EnumIdx++;
//				EnumID++;
//			}
//		}
//#endif
	}

	static void RegisterMemberVirtual(asIScriptEngine* Engine, CMwClassInfo* Class, CMwMemberInfo* Member)
	{
		using enum CMwMemberInfo::EType;

		RegisterVirtualSetter(Engine, Class, Member);

		if (auto asClass = Engine->GetTypeInfoByName(Class->GetName().c_str()))
		{
			if (asClass->GetMethodByName(std::format(
				"get_{}",
				Member->GetName()
			).c_str()))
			{
				return;
			}
		}

		switch (Member->MemberType)
		{
		case CLASS:
			Engine->RegisterObjectMethod(
				Class->GetName().c_str(),
				std::format(
					"{}@ get_{}() property",
					((CMwMemberInfoClass*)Member)->ClassInfo->GetName(), Member->GetName()
				).c_str(),
				asFUNCTION(VirtualGetAs),
				asCALL_GENERIC,
				Member
			);
			break;
		case NATURAL:
			Engine->RegisterObjectMethod(
				Class->GetName().c_str(),
				std::format(
					"uint get_{}() property",
					Member->GetName()
				).c_str(),
				asFUNCTION(VirtualGetAs),
				asCALL_GENERIC,
				Member
			);
			break;
		case STRING:
#ifndef TM1
		case STRINGINT:
#endif
			GameStrings::Register(Engine, Class, Member);
			break;
		default:
			break;
		}
	}

	static void RegisterMemberNormal(asIScriptEngine* Engine, CMwClassInfo* Class, CMwMemberInfo* Member)
	{
		using enum CMwMemberInfo::EType;

		switch (Member->MemberType)
		{
		case CLASS:
		{
			Engine->RegisterObjectProperty(
				Class->GetName().c_str(),
				std::format(
					"{}@ {}",
					((CMwMemberInfoClass*)Member)->ClassInfo->GetName(), Member->GetName()
				).c_str(),
				Member->MemberOffset
			);
			break;
		}
		case CLASSARRAY:
		{
			Engine->RegisterObjectProperty(
				Class->GetName().c_str(),
				std::format(
					"MwFastArray<{}@> {}",
					((CMwMemberInfoClassArray*)Member)->ArrayClassInfo->GetName(), Member->GetName()
				).c_str(),
				Member->MemberOffset
			);
			break;
		}
		case BOOL:
		case INT:
		case NATURAL:
		case REAL:
		// The math types have the same layout as the game's (see Gecko/Exports/Math/Math.h)
		case VEC2:
		case VEC3:
		case VEC4:
		case ISO3:
		case ISO4:
#ifdef TMCN
		case INT2:
		case INT3:
		case NAT2:
		case NAT3:
#endif
		case COLOR:
		case NATURALRANGE:
		case REALRANGE:
		case BOOLARRAY:
		case COLORARRAY:
		case INTARRAY:
		case NATURALARRAY:
		case REALARRAY:
		case VEC2ARRAY:
		case VEC3ARRAY:
		case VEC4ARRAY:
		case ISO3ARRAY:
		case ISO4ARRAY:
		case BOOLBUFFER:
		case COLORBUFFER:
		case INTBUFFER:
		case NATURALBUFFER:
		case REALBUFFER:
		case VEC2BUFFER:
		case VEC3BUFFER:
		case VEC4BUFFER:
		case ISO3BUFFER:
		case ISO4BUFFER:
		case STRINGARRAY:
		case STRINGBUFFER:
#ifndef TM1
		case STRINGINTARRAY:
		case STRINGINTBUFFER:
#endif
#ifdef TMCN
		case INT2ARRAY:
		case INT3ARRAY:
		case NAT2ARRAY:
		case NAT3ARRAY:
		case INT2BUFFER:
		case INT3BUFFER:
		case NAT2BUFFER:
		case NAT3BUFFER:
#endif
		{
			Engine->RegisterObjectProperty(
				Class->GetName().c_str(),
				std::format(
					"{} {}",
					g_MemberTypeSignatures[Member->MemberType], 
					Member->GetName()
				).c_str(),
				Member->MemberOffset
			);
			break;
		}
		case INTRANGE:
		{
			Engine->RegisterObjectProperty(Class->GetName().c_str(), std::format("int {}", Member->GetName()).c_str(), Member->MemberOffset);
			break;
		}
		case CLASSBUFFER:
		{
			Engine->RegisterObjectProperty(
				Class->GetName().c_str(),
				std::format(
					"MwFastBuffer<{}@> {}",
					((CMwMemberInfoClassArray*)Member)->ArrayClassInfo->GetName(), Member->GetName()
				).c_str(),
				Member->MemberOffset
			);
			break;
		}
		case STRING:
#ifndef TM1
		case STRINGINT:
#endif
		{
			GameStrings::Register(Engine, Class, Member);
			break;
		}
		default:
		{
			break;
		}
		}
	}

	// Registers all classes available from the engine manager.
	void RegistrarFromTm(asIScriptEngine* Engine)
	{
		CMwEngineManager* EngineMgr = gTwinkie.TrackmaniaMgr.GetEngineManager();

		for (auto& TmEngine : EngineMgr->Engines)
		{
			if (!TmEngine) continue;

#ifdef GAMEBOX
			for (uint32_t ClassIdx = 0; ClassIdx < TmEngine->ClassesAmount; ClassIdx++)
#else
			for (auto& Class : TmEngine->Classes)
#endif
			{
#ifdef GAMEBOX
				auto Class = TmEngine->Classes[ClassIdx];
#endif
				if (!Class) continue;

				RegisterClass(Engine, Class);
			}
		}
		for (auto& TmEngine : EngineMgr->Engines)
		{
			if (!TmEngine) continue;

#ifdef GAMEBOX
			for (uint32_t ClassIdx = 0; ClassIdx < TmEngine->ClassesAmount; ClassIdx++)
#else
			for (auto& Class : TmEngine->Classes)
#endif
			{
#ifdef GAMEBOX
				auto Class = TmEngine->Classes[ClassIdx];
#endif
				if (!Class) continue;

				CMwClassInfo* Parent = Class;
				while (Parent)
				{
					for (uint32_t MemberIdx = 0; MemberIdx < Parent->MembersAmount; MemberIdx++)
					{
						CMwMemberInfo* Member = Parent->Members[MemberIdx];

						if (auto TypeInfo = Engine->GetTypeInfoByName(Class->GetName().c_str()))
						{
							bool SkipMember = false;
							for (uint32_t AsMemberIdx = 0; AsMemberIdx < TypeInfo->GetPropertyCount(); AsMemberIdx++)
							{
								const char* AsMemberName = nullptr;
								TypeInfo->GetProperty(AsMemberIdx, &AsMemberName);

								if (strcmp(AsMemberName, Member->GetName().c_str()) == 0)
								{
									SkipMember = true;
									break;
								}
							}
							std::string MemberName = Member->GetName();
							if (TypeInfo->GetMethodByName(("get_" + MemberName).c_str()) || TypeInfo->GetMethodByName(("set_" + MemberName).c_str()))
								SkipMember = true;
							if (SkipMember) continue;
						}

						// Non-virtual
						// TODO: Use Member->MemberFlags0/1 to know if to use VirtualGet/Set.
						if (Member->MemberOffset <= 32767 && !TwinkTrackmania::IsVirtualGet(Member))
							RegisterMemberNormal(Engine, Class, Member);
						else
							RegisterMemberVirtual(Engine, Class, Member);
					}
					Parent = Parent->ParentClassInfo;
				}
			}
		}
		for (auto& TmEngine : EngineMgr->Engines)
		{
			if (!TmEngine) continue;

#ifdef GAMEBOX
			for (uint32_t ClassIdx = 0; ClassIdx < TmEngine->ClassesAmount; ClassIdx++)
#else
			for (auto& Class : TmEngine->Classes)
#endif
			{
#ifdef GAMEBOX
				auto Class = TmEngine->Classes[ClassIdx];
#endif
				if (!Class) continue;

				CMwClassInfo* Parent = Class->ParentClassInfo;
				bool HasMwNodParent = false;
				Parent = Class->ParentClassInfo;
				while (Parent)
				{
					Engine->RegisterObjectMethod(
						Parent->GetName().c_str(),
						std::format(
							"{}@ opCast()",
							Class->GetName()
						).c_str(),
						asFUNCTION(Caster),
						asCALL_GENERIC,
						Class
					);

					Engine->RegisterObjectMethod(
						Class->GetName().c_str(),
						std::format(
							"{}@ opCast()",
							Parent->GetName()
						).c_str(),
						asFUNCTION(Caster),
						asCALL_GENERIC,
						Parent
					);

					if (Parent->GetName() == "CMwNod")
						HasMwNodParent = true;

					Parent = Parent->ParentClassInfo;
				}
				Parent = Class->ParentClassInfo;
				while (Parent && HasMwNodParent)
				{
					if (Engine->GetTypeInfoByDecl(Class->GetName().c_str())->GetMethodByDecl("CMwNod@ opImplCast()"))
					{
						Parent = Parent->ParentClassInfo;
						continue;
					}

					Engine->RegisterObjectMethod(
						Class->GetName().c_str(),
						"CMwNod@ opImplCast()",
						asFUNCTION(Caster),
						asCALL_GENERIC,
						Parent
					);

					Parent = Parent->ParentClassInfo;
				}
			}
#ifdef _DEBUG
			std::cout << "Registered engine " << TmEngine->EngineName << " (ID 0x" << std::hex << TmEngine->EngineID << std::dec << ")\n";
#endif
		}
	}

	void Registrar(asIScriptEngine* Engine)
	{
		BaseTypes::RegistrarBaseTypes(Engine);

		AllowStringSubtypes = true;
		GameStrings::RegisterArrays<CFastString>(Engine);
#ifndef TM1
		GameStrings::RegisterArrays<CFastStringInt>(Engine);
#endif
		AllowStringSubtypes = false;

		RegistrarFromTm(Engine);

		RegisterAll(Engine, s_Exports);
	}

	void Cleanup()
	{
	}
}