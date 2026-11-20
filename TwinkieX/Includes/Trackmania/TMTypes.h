#include <Utils/Offsets.h>
#pragma once

struct Vec2
{
	float x, y;

	Vec2 operator+(Vec2 B)
	{
		return Vec2(this->x + B.x, this->y + B.y);
	}

	Vec2 operator-(Vec2 B)
	{
		return Vec2(this->x - B.x, this->y - B.y);
	}

	Vec2 operator*(float F)
	{
		return Vec2(this->x * F, this->y * F);
	}

	operator ImVec2()
	{
		return ImVec2(this->x, this->y);
	}
};

struct Vec3
{
	float x, y, z;
};

struct Vec4
{
	float x, y, z, w;
};

using Quat = Vec4;

struct Mat3
{
	union
	{
		float M[9];
		struct
		{
			Vec3 X;
			Vec3 Y;
			Vec3 Z;
		};
	};
};

struct Iso4
{
	Mat3 Rotation;
	Vec3 Position;
};

class CMwNod;
class CMwStack;

using ActionFn = void(
#ifdef X86
	__thiscall
#endif
	*)(CMwNod*);

// Resizable non-templated array type, with 32-bit-sized elements, similar to CFastBuffer
struct CFastBufferGen
{
#ifdef TMCN
	void* Ptr = nullptr;
	uint32_t Size = 0;
	uint32_t Capacity = 0;
#elif defined(GAMEBOX)
	uint32_t Size = 0;
	void* Ptr = nullptr;
	uint32_t Capacity = 0;
#endif

	void* operator[](size_t Idx)
	{
		return (uint32_t*)Ptr + Idx;
	}

	void* Get(size_t Idx, size_t SizeOfElem)
	{
		return &((uint8_t*)Ptr)[Idx * SizeOfElem];
	}

	void* begin() { return Ptr; }
	void* end() { return (uint32_t*)Ptr + Size; }

	const void* begin() const { return Ptr; }
	const void* end() const { return (uint32_t*)Ptr + Size; }
};

// Resizable array type used by the game
template <typename T>
struct CFastBuffer
{
#ifdef TMCN
	T* Ptr = nullptr;
	uint32_t Size = 0;
	uint32_t Capacity = 0;
#elif defined(GAMEBOX)
	uint32_t Size = 0;
	T* Ptr = nullptr;
	uint32_t Capacity = 0;
#endif

	T* operator[](size_t Idx)
	{
		return Ptr + Idx;
	}

	T* begin() { return Ptr; }
	T* end() { return Ptr + Size; }

	const T* begin() const { return Ptr; }
	const T* end() const { return Ptr + Size; }

	operator CFastBufferGen ()
	{
		return *reinterpret_cast<CFastBufferGen*>(this);
	}
};

// Single-size non-templated 32-bit-size element array type, similar to CFastArray
struct CFastArrayGen
{
#ifdef MANIAPLANET
	void* Ptr = nullptr;
	uint32_t Size = 0;
#else
	uintptr_t vf;
	uint32_t Size = 0;
	void* Ptr = nullptr;
#endif

	void* operator[](size_t Idx)
	{
		return (uint8_t*)Ptr + Idx;
	}

	void* Get(size_t Idx, size_t SizeOfElem)
	{
		return &((uint8_t*)Ptr)[Idx * SizeOfElem];
	}

	void* begin() { return Ptr; }
	void* end() { return (uint32_t*)Ptr + Size; }

	const void* begin() const { return Ptr; }
	const void* end() const { return (uint32_t*)Ptr + Size; }
};

// Single-size array type used by the game
template <typename T>
struct CFastArray
{
#ifdef MANIAPLANET
	T* Ptr = nullptr;
	uint32_t Size = 0;
#else
	uintptr_t vf;
	uint32_t Size = 0;
	T* Ptr = nullptr;
#endif

	T* operator[](size_t Idx)
	{
		return Ptr + Idx;
	}

	T* begin() { return Ptr; }
	T* end() { return Ptr + Size; }

	const T* begin() const { return Ptr; }
	const T* end() const { return Ptr + Size; }

	operator CFastArrayGen ()
	{
		return *reinterpret_cast<CFastArrayGen*>(this);
	}
};

// Used for parameters to PROCs
struct CMwParam 
{
	void* Ptr;
};

#ifdef TMCN
// Information about a class' member
// This class is exclusive to TMCN
struct CMwMemberInfo
{
	enum EType
	{
		ACTION = 0,

		BOOL = 1,
		BOOLARRAY = 2,
		BOOLBUFFER = 3,
		BOOLBUFFERCAT = 4,
		BOOLSARRAY = 5,
		BOOLARRAYMAXDYN = 6,
		BOOLSTRIDEDARRAY = 7,
		BOOLARRAYINPLACE = 8,
		BOOLARRAYINPLACEDYN = 9,

		CLASS = 10,
		CLASSARRAY = 11,
		CLASSBUFFER = 12,
		CLASSBUFFERCAT = 13,
		CLASSSARRAY = 14,
		CLASSARRAYMAXDYN = 15,
		CLASSSTRIDEDARRAY = 16,
		CLASSARRAYINPLACE = 17,
		CLASSARRAYINPLACEDYN = 18,

		COLOR = 19,
		COLORARRAY = 20,
		COLORBUFFER = 21,
		COLORBUFFERCAT = 22,
		COLORSARRAY = 23,
		COLORARRAYMAXDYN = 24,
		COLORSTRIDEDARRAY = 25,
		COLORARRAYINPLACE = 26,
		COLORARRAYINPLACEDYN = 27,

		ENUM = 28,
		ENUMARRAY = 29,
		ENUMBUFFER = 30,
		ENUMBUFFERCAT = 31,
		ENUMSARRAY = 32,
		ENUMARRAYMAXDYN = 33,
		ENUMSTRIDEDARRAY = 34,
		ENUMARRAYINPLACE = 35,
		ENUMARRAYINPLACEDYN = 36,

		INT = 37,
		INTARRAY = 38,
		INTBUFFER = 39,
		INTBUFFERCAT = 40,
		INTSARRAY = 41,
		INTARRAYMAXDYN = 42,
		INTSTRIDEDARRAY = 43,
		INTARRAYINPLACE = 44,
		INTARRAYINPLACEDYN = 45,

		INTRANGE = 46, // INT with a min/max

		ISO4 = 47,
		ISO4ARRAY = 48,
		ISO4BUFFER = 49,
		ISO4BUFFERCAT = 50,
		ISO4SARRAY = 51,
		ISO4ARRAYMAXDYN = 52,
		ISO4STRIDEDARRAY = 53,
		ISO4ARRAYINPLACE = 54,
		ISO4ARRAYINPLACEDYN = 55,

		ISO3 = 56,
		ISO3ARRAY = 57,
		ISO3BUFFER = 58,
		ISO3BUFFERCAT = 59,
		ISO3SARRAY = 60,
		ISO3ARRAYMAXDYN = 61,
		ISO3STRIDEDARRAY = 62,
		ISO3ARRAYINPLACE = 63,
		ISO3ARRAYINPLACEDYN = 64,

		ID = 65, // CMwId
		IDARRAY = 66,
		IDBUFFER = 67,
		IDBUFFERCAT = 68,
		IDSARRAY = 69,
		IDARRAYMAXDYN = 70,
		IDSTRIDEDARRAY = 71,
		IDARRAYINPLACE = 72,
		IDARRAYINPLACEDYN = 73,

		NATURAL = 74,
		NATURALARRAY = 75,
		NATURALBUFFER = 76,
		NATURALBUFFERCAT = 77,
		NATURALSARRAY = 78,
		NATURALARRAYMAXDYN = 79,
		NATURALSTRIDEDARRAY = 80,
		NATURALARRAYINPLACE = 81,
		NATURALARRAYINPLACEDYN = 82,

		NATURALRANGE = 83, // NATURAL with a min/max

		REAL = 84,
		REALARRAY = 85,
		REALBUFFER = 86,
		REALBUFFERCAT = 87,
		REALSARRAY = 88,
		REALARRAYMAXDYN = 89,
		REALSTRIDEDARRAY = 90,
		REALARRAYINPLACE = 91,
		REALARRAYINPLACEDYN = 92,

		REALRANGE = 93, // REAL with a min/max

		STRING = 94,
		STRINGARRAY = 95,
		STRINGBUFFER = 96,
		STRINGBUFFERCAT = 97,
		STRINGSARRAY = 98,
		STRINGARRAYMAXDYN = 99,
		STRINGSTRIDEDARRAY = 100,
		STRINGARRAYINPLACE = 101,
		STRINGARRAYINPLACEDYN = 102,

		STRINGINT = 103, // Wide string
		STRINGINTARRAY = 104,
		STRINGINTBUFFER = 105,
		STRINGINTBUFFERCAT = 106,
		STRINGINTSARRAY = 107,
		STRINGINTARRAYMAXDYN = 108,
		STRINGINTSTRIDEDARRAY = 109,
		STRINGINTARRAYINPLACE = 110,
		STRINGINTARRAYINPLACEDYN = 111,

		VEC2 = 112,
		VEC2ARRAY = 113,
		VEC2BUFFER = 114,
		VEC2BUFFERCAT = 115,
		VEC2SARRAY = 116,
		VEC2ARRAYMAXDYN = 117,
		VEC2STRIDEDARRAY = 118,
		VEC2ARRAYINPLACE = 119,
		VEC2ARRAYINPLACEDYN = 120,

		VEC3 = 121,
		VEC3ARRAY = 122,
		VEC3BUFFER = 123,
		VEC3BUFFERCAT = 124,
		VEC3SARRAY = 125,
		VEC3ARRAYMAXDYN = 126,
		VEC3STRIDEDARRAY = 127,
		VEC3ARRAYINPLACE = 128,
		VEC3ARRAYINPLACEDYN = 129,

		VEC4 = 130,
		VEC4ARRAY = 131,
		VEC4BUFFER = 132,
		VEC4BUFFERCAT = 133,
		VEC4SARRAY = 134,
		VEC4ARRAYMAXDYN = 135,
		VEC4STRIDEDARRAY = 136,
		VEC4ARRAYINPLACE = 137,
		VEC4ARRAYINPLACEDYN = 138,

		INT2 = 139,
		INT2ARRAY = 140,
		INT2BUFFER = 141,
		INT2BUFFERCAT = 142,
		INT2SARRAY = 143,
		INT2ARRAYMAXDYN = 144,
		INT2STRIDEDARRAY = 145,
		INT2ARRAYINPLACE = 146,
		INT2ARRAYINPLACEDYN = 147,

		INT3 = 148,
		INT3ARRAY = 149,
		INT3BUFFER = 150,
		INT3BUFFERCAT = 151,
		INT3SARRAY = 152,
		INT3ARRAYMAXDYN = 153,
		INT3STRIDEDARRAY = 154,
		INT3ARRAYINPLACE = 155,
		INT3ARRAYINPLACEDYN = 156,

		NAT2 = 157,
		NAT2ARRAY = 158,
		NAT2BUFFER = 159,
		NAT2BUFFERCAT = 160,
		NAT2SARRAY = 161,
		NAT2ARRAYMAXDYN = 162,
		NAT2STRIDEDARRAY = 163,
		NAT2ARRAYINPLACE = 164,
		NAT2ARRAYINPLACEDYN = 165,

		NAT3 = 166,
		NAT3ARRAY = 167,
		NAT3BUFFER = 168,
		NAT3BUFFERCAT = 169,
		NAT3SARRAY = 170,
		NAT3ARRAYMAXDYN = 171,
		NAT3STRIDEDARRAY = 172,
		NAT3ARRAYINPLACE = 173,
		NAT3ARRAYINPLACEDYN = 174,

		QUAT = 175,
		QUATARRAY = 176,
		QUATBUFFER = 177,
		QUATBUFFERCAT = 178,
		QUATSARRAY = 179,
		QUATARRAYMAXDYN = 180,
		QUATSTRIDEDARRAY = 181,
		QUATARRAYINPLACE = 182,
		QUATARRAYINPLACEDYN = 183,

		PROC = 184, // Method, see CMwMemberInfoProc

		DATAREF = 185, // 24-byte data reference

		CLASSNOTPERSISTENT = 186, // Raw class pointer, not reference-counted. Used by the MetaNotPersistent classes
		CLASSNOTPERSISTENTARRAY = 187,
		CLASSNOTPERSISTENTBUFFER = 188,
		CLASSNOTPERSISTENTBUFFERCAT = 189,
		CLASSNOTPERSISTENTSARRAY = 190,
		CLASSNOTPERSISTENTARRAYMAXDYN = 191,
		CLASSNOTPERSISTENTSTRIDEDARRAY = 192,
		CLASSNOTPERSISTENTARRAYINPLACE = 193,
		CLASSNOTPERSISTENTARRAYINPLACEDYN = 194,

		AGGREG = 195, // Class/struct embedded by value
		AGGREGARRAY = 196,
		AGGREGBUFFER = 197,
		AGGREGBUFFERCAT = 198,
		AGGREGSARRAY = 199,
		AGGREGARRAYMAXDYN = 200,
		AGGREGSTRIDEDARRAY = 201,
		AGGREGARRAYINPLACE = 202,
		AGGREGARRAYINPLACEDYN = 203,

		NAT8 = 204,
		NAT8ARRAY = 205,
		NAT8BUFFER = 206,
		NAT8BUFFERCAT = 207,
		NAT8SARRAY = 208,
		NAT8ARRAYMAXDYN = 209,
		NAT8STRIDEDARRAY = 210,
		NAT8ARRAYINPLACE = 211,
		NAT8ARRAYINPLACEDYN = 212,

		NAT16 = 213,
		NAT16ARRAY = 214,
		NAT16BUFFER = 215,
		NAT16BUFFERCAT = 216,
		NAT16SARRAY = 217,
		NAT16ARRAYMAXDYN = 218,
		NAT16STRIDEDARRAY = 219,
		NAT16ARRAYINPLACE = 220,
		NAT16ARRAYINPLACEDYN = 221,

		INT8 = 222,
		INT8ARRAY = 223,
		INT8BUFFER = 224,
		INT8BUFFERCAT = 225,
		INT8SARRAY = 226,
		INT8ARRAYMAXDYN = 227,
		INT8STRIDEDARRAY = 228,
		INT8ARRAYINPLACE = 229,
		INT8ARRAYINPLACEDYN = 230,

		INT16 = 231,
		INT16ARRAY = 232,
		INT16BUFFER = 233,
		INT16BUFFERCAT = 234,
		INT16SARRAY = 235,
		INT16ARRAYMAXDYN = 236,
		INT16STRIDEDARRAY = 237,
		INT16ARRAYINPLACE = 238,
		INT16ARRAYINPLACEDYN = 239,

		NODREF = 240,
		NODREFARRAY = 241,
		NODREFBUFFER = 242,
		NODREFBUFFERCAT = 243,
		NODREFSARRAY = 244,
		NODREFARRAYMAXDYN = 245,
		NODREFSTRIDEDARRAY = 246,
		NODREFARRAYINPLACE = 247,
		NODREFARRAYINPLACEDYN = 248,

		ENUM8 = 249,
		ENUM8ARRAY = 250,
		ENUM8BUFFER = 251,
		ENUM8BUFFERCAT = 252,
		ENUM8SARRAY = 253,
		ENUM8ARRAYMAXDYN = 254,
		ENUM8STRIDEDARRAY = 255,
		ENUM8ARRAYINPLACE = 256,
		ENUM8ARRAYINPLACEDYN = 257,

		ENUM16 = 258,
		ENUM16ARRAY = 259,
		ENUM16BUFFER = 260,
		ENUM16BUFFERCAT = 261,
		ENUM16SARRAY = 262,
		ENUM16ARRAYMAXDYN = 263,
		ENUM16STRIDEDARRAY = 264,
		ENUM16ARRAYINPLACE = 265,
		ENUM16ARRAYINPLACEDYN = 266,

		RGBACOLOR = 267,
		RGBACOLORARRAY = 268,
		RGBACOLORBUFFER = 269,
		RGBACOLORBUFFERCAT = 270,
		RGBACOLORSARRAY = 271,
		RGBACOLORARRAYMAXDYN = 272,
		RGBACOLORSTRIDEDARRAY = 273,
		RGBACOLORARRAYINPLACE = 274,
		RGBACOLORARRAYINPLACEDYN = 275,

		FASTNODPOOL = 276,

		MAX = 512
	};

	std::string GetName() const;

	enum EFlags 
	{
		GET = 0b00000001,
		SET = 0b00000010,
		ADD = 0b00000100,
		SUB = 0b00001000,
		VIRTUAL_GET = 0b00010000,
		VIRTUAL_SET = 0b00100000,
		VIRTUAL_ADD = 0b01000000,
		VIRTUAL_SUB = 0b10000000,
	};
	
	// Type of the member
	EType MemberType;

	// Member ID, unique between classes
	uint32_t MemberID;

	// I have no idea
	CMwParam* pParam;

	// Offset of the member in the class instance, -1 for methods and virtual members
	uint32_t MemberOffset;

	uint32_t Pad0;

	// Name of the member
	const char* MemberName;

	// Unknown pointer
	uintptr_t p;

	// Flags for the member, see (EFlags)
	uint32_t MemberFlags;
	// Secondary flags for the member, see (EFlags)
	uint32_t MemberFlags2;
};

// Info about a class' enum
struct CMwEnumInfo
{
	const char* EnumName;
	uint32_t Pad;
	uint32_t EnumSize;
	uint32_t EnumOwnerClassId;
	uint32_t EnumValueCount;
	const char** EnumValueNames;

	const char** begin() { return EnumValueNames; }
	const char** end() { return EnumValueNames + EnumValueCount; }
};

// Information about a class
// This class is exclusive to TMCN
struct CMwClassInfo
{
	uintptr_t* vf;

	// Class name
	const char* ClassName;

	uint32_t unknown0;
	// Size of the class in bytes
	uint32_t Size;

	// Class ID
	uint32_t ClassID;

	uint32_t unknown2;

	// Parent class' info, null if none
	CMwClassInfo* ParentClassInfo;

	// Next class info, usually has nothing to do with this class
	CMwClassInfo* NextClassInfo;
	// Pointer to the class' constructor
	ActionFn CtorFn;

	// Padding for unknown data
	uint64_t Padding[12];

	char** Description;

	uint64_t Padding2;
	CMwEnumInfo** ClassEnums;
	uint32_t ClassEnumsCount;
	uint32_t Padding3;
	uint64_t Padding4[2];

	// File extension for this class
	char* FileExtName;

	// List of member infos
	CMwMemberInfo** Members;
	// Number of member infos in the list
	uint32_t MembersAmount;

	// Iterator function for the beginning of the members array
	CMwMemberInfo** begin();
	// Iterator function for the end of the members array
	CMwMemberInfo** end();

	// Const iterator function for the beginning of the members array
	CMwMemberInfo** begin() const;
	// Const iterator function for the end of the members array
	CMwMemberInfo** end() const;

	std::string GetName() const;

	uint32_t Pad0;
};

// Info of a CLASS type member
struct CMwMemberInfoClass : CMwMemberInfo
{
	ActionFn InstantiationFn;
	CMwClassInfo* ClassInfo;
	uint32_t ClassID;
	uint32_t Pad;
};

struct CMwMemberInfoRealRange : CMwMemberInfo
{
	uint8_t Padding[8];

	float ValueMin;
	float ValueMax;
};

struct CMwMemberInfoIntRange : CMwMemberInfo
{
	uint8_t Padding[8];

	// KEEP THESE AS int

	int ValueMin;
	int ValueMax;
};

struct CMwMemberInfoEnum : CMwMemberInfo
{
	uint8_t Padding[8];

	const char* EnumTypeName;
	int32_t EnumValueNamesLength;
	uint32_t Padding2;
	const char** EnumValueNames;

	uint64_t Unknown;
};

// Info for a member who is an array of CLASSes (like CLASSBUFFER)
struct CMwMemberInfoClassArray : CMwMemberInfo
{
	uint8_t Padding[16];

	// If member name is something like "Tracks", this would be "Track" instead. Could be null.
	const char* ElementNameSingular;

	uint64_t Padding2;

	CMwClassInfo* ArrayClassInfo;
};

struct CMwMemberInfoProc : CMwMemberInfo
{
	// Returns 0 on success, 2 on invalid arguments and 8 on a script error
	using ProcFn = uint32_t(*)(CMwNod* Nod, CMwStack* Stack);

	struct Type
	{
		// How AdditionalInfo is interpreted
		enum EKind : uint32_t
		{
			VALUE = 0,
			CLASS = 1,
			ENUM = 2
		} Kind;
		uint32_t AdditionalInfo;
	};

	enum EParamFlags : uint8_t
	{
		PARAM_IN = 1,
		PARAM_OUT = 2
	};

	struct Param
	{
		Type Type;
		const char* Name;
		uint8_t Flags;
	};

	uint64_t Padding;
	const char* ProcName;
	// Called with the same arguments when MethodItself is null
	ProcFn MethodAlt;
	ProcFn MethodItself;

	uint32_t ParamCount;
	uint32_t Padding2;
	Type* Params;
	const char** ParamNames;
	uint8_t* ParamFlags;

	std::vector<Param> GetParams()
	{
		std::vector<Param> OutParams;

		for (size_t ParamIdx = 0; ParamIdx < ParamCount; ParamIdx++)
		{
			OutParams.push_back(Param{ this->Params[ParamIdx], ParamNames[ParamIdx], ParamFlags[ParamIdx]});
		}

		return OutParams;
	}
};

struct CFastStringInt
{
	wchar_t* CStr;
	uint32_t Count;
};

struct CFastString
{
	char* CStr;
	uint32_t Count;
};

// Pool of nods allocated in fixed-size blocks
struct CFastNodPool
{
	void* Vtable;
	// Amount of nods in Nods. The game uses this one instead of Nods.Size
	uint32_t Count;
	uint32_t Pad;
	uint64_t Unknown;
	CFastBuffer<uint8_t*> Blocks;
	CFastBuffer<CMwNod*> Nods;
};
#else
#ifndef TM1
struct CFastStringInt
{
	uint32_t Count;
	wchar_t* CStr;
};

struct CFastString
{
	uint32_t Count;
	char* CStr;
};
#else
struct CFastStringInt
{
	uint32_t Pad;
	uint32_t Count;
	char* CStr;
};

struct CFastString
{
	void* Vtable;
	// Includes the null terminator, so an empty string has a Count of 1
	uint32_t Count;
	char* CStr;
};
#endif

// Information about a class' member
// This class is shared between TM1, TMO, TMS, TMSX and ESWC
struct CMwMemberInfo
{
	enum EType
	{
		ACTION,
		BOOL,
		BOOLARRAY,
		BOOLBUFFER,
		BOOLBUFFERCAT,
		CLASS,
		CLASSARRAY,
		CLASSBUFFER,
		CLASSBUFFERCAT,
		COLOR,
		COLORARRAY,
		COLORBUFFER,
		COLORBUFFERCAT,
		ENUM,
		INT,
		INTARRAY,
		INTBUFFER,
		INTBUFFERCAT,
		INTRANGE,
		ISO4,
		ISO4ARRAY,
		ISO4BUFFER,
		ISO4BUFFERCAT,
		ISO3,
		ISO3ARRAY,
		ISO3BUFFER,
		ISO3BUFFERCAT,
#ifdef TMU
		ID,
		IDARRAY,
		IDBUFFER,
		IDBUFFERCAT,
#endif
		NATURAL,
		NATURALARRAY,
		NATURALBUFFER,
		NATURALBUFFERCAT,
		NATURALRANGE,
		REAL,
		REALARRAY,
		REALBUFFER,
		REALBUFFERCAT,
		REALRANGE,
		STRING,
		STRINGARRAY,
		STRINGBUFFER,
		STRINGBUFFERCAT,
#ifndef TM1
		STRINGINT,
		STRINGINTARRAY,
		STRINGINTBUFFER,
		STRINGINTBUFFERCAT,
#endif
		VEC2,
		VEC2ARRAY,
		VEC2BUFFER,
		VEC2BUFFERCAT,
		VEC3,
		VEC3ARRAY,
		VEC3BUFFER,
		VEC3BUFFERCAT,
		VEC4,
		VEC4ARRAY,
		VEC4BUFFER,
		VEC4BUFFERCAT,
		QUAT,
		QUATARRAY,
		QUATBUFFER,
		QUATBUFFERCAT,
		PROC,
		REFBUFFER,

		MAX
	};

	enum EFlags
	{
		GET = 0b00000001,
		SET = 0b00000010,
		ADD = 0b00000100,
		SUB = 0b00001000,
		VIRTUAL_GET = 0b00010000,
		VIRTUAL_SET = 0b00100000,
		VIRTUAL_ADD = 0b01000000,
		VIRTUAL_SUB = 0b10000000,
	};

	std::string GetName() const;

	// Type of the member
	EType MemberType;

	// Member ID, unique between classes
	uint32_t MemberID;

	// I have no idea
	CMwParam* pParam;

	// Offset of the member in the class instance, -1 for methods and virtual members
	uint32_t MemberOffset;

	// Member name
	const char* MemberName;

	// Flags for the member, see (EFlags)
	uint32_t MemberFlags;
	// Secondary flags for the member, see (EFlags)
	uint32_t MemberFlags2;
};

// Information about a class' member
// This class is shared between TM1, TMO, TMS, TMSX and ESWC
struct CMwClassInfo
{
	// Virtual function table pointer
	ActionFn* vftable;

	// Class ID
	uint32_t ClassID;

	// Parent class' info, null if none
	CMwClassInfo* ParentClassInfo;

	uint32_t unknown0;
	uint32_t unknown1;
	
	// Class name
	char* ClassName;

	// Next class info, usually has nothing to do with this class
	CMwClassInfo* NextClassInfo;
#ifdef TM1
	uint32_t unknown2;
#endif
	// Pointer to the class' constructor
	ActionFn CtorFn;

	// List of member infos
	CMwMemberInfo** Members;
	// Number of member infos in the list
	uint32_t MembersAmount;

	// Iterator function for the beginning of the members array
	CMwMemberInfo** begin();
	// Iterator function for the end of the members array
	CMwMemberInfo** end();

	// Const iterator function for the beginning of the members array
	CMwMemberInfo** begin() const;
	// Const iterator function for the end of the members array
	CMwMemberInfo** end() const;

	std::string GetName() const;
};

// Info of a CLASS type member
struct CMwMemberInfoClass : CMwMemberInfo
{
#ifndef TMU
	uint32_t Pad0;
#else
	CMwClassInfo* ClassInfo;
#endif

	uint32_t Pad1;

#ifndef TMU
	CMwClassInfo* ClassInfo;
#else
	uint32_t Pad0;
#endif

	uint32_t Pad2;
};

// Info for a member who is an array of CLASSes (like CLASSBUFFER)
struct CMwMemberInfoClassArray : CMwMemberInfo
{
	uint32_t Pad0;

	uint32_t Pad1;

	uint32_t Pad2;

#ifndef TMU
	const char* ElementNameSingular;
#else
	CMwClassInfo* ArrayClassInfo;
#endif

	uint32_t Pad3;

#ifndef TMU
	CMwClassInfo* ArrayClassInfo;
#else
	uint32_t Pad4;
#endif
};

// Info of a PROC type member
struct CMwMemberInfoProc : CMwMemberInfo
{
	// Called by CMwNod::Param_Set for PROCs that aren't virtual
	using ProcFn = void(__thiscall*)(CMwNod*, CMwStack*);

	struct Type
	{
		// How AdditionalInfo is interpreted
		enum EKind : uint32_t
		{
			// A plain value. AdditionalInfo is its member type (CMwMemberInfo::EType)
			VALUE = 0,
			// A nod. AdditionalInfo is its class ID
			CLASS = 1,
			// An enum. AdditionalInfo is the enum's type ID
			ENUM = 2
		} Kind;
		uint32_t AdditionalInfo;
	};

	enum EParamFlags : uint8_t
	{
		PARAM_IN = 1,
		// Written by the PROC. The first one is the return value
		PARAM_OUT = 2
	};

	struct Param
	{
		Type Type;
		const char* Name;
		uint8_t Flags;
	};

#ifndef TMU
	// Except in TMU, member infos are longer than CMwMemberInfo, so the real flags are here instead of MemberFlags
	uint32_t ProcFlags;
	uint32_t ProcFlags2;
#endif

	// Null for virtual PROCs (VIRTUAL_SET), which are called through CMwNod::VirtualParam_Set instead
	ProcFn Method;

	uint32_t ParamCount;
	// A member type if below 0x1001000, otherwise a class ID, or an enum ID if its lowest 12 bits aren't 0
	uint32_t* Params;
	const char** ParamNames;
	uint8_t* ParamFlags;

	std::vector<Param> GetParams()
	{
		std::vector<Param> OutParams;

		for (size_t ParamIdx = 0; ParamIdx < ParamCount; ParamIdx++)
		{
			uint32_t ParamType = Params[ParamIdx];
			Type::EKind Kind = ParamType < 0x1001000 ? Type::VALUE : (ParamType & 0xFFF) ? Type::ENUM : Type::CLASS;

			OutParams.push_back(Param{ Type{ Kind, ParamType }, ParamNames[ParamIdx], ParamFlags[ParamIdx] });
		}

		return OutParams;
	}

	// The flags the game checks when calling the PROC
	uint32_t GetFlags() const
	{
#ifndef TMU
		return ProcFlags;
#else
		return MemberFlags;
#endif
	}
};

#endif

// Info of an ACTION type member
struct CMwMemberInfoAction : CMwMemberInfo
{
	uint8_t Padding[8];

	// The action function itself
	ActionFn Action;
};

// The class used by the app's input port (CTrackMania.InputPort)
struct CInputPort
{
	uint8_t Padding[O_M_CINPUTPORT_CONNECTEDDEVICES];
	CFastArray<CMwNod*> ConnectedDevices;
	uint8_t Padding2[O_M_CINPUTPORT_ISFOCUSED - O_M_CINPUTPORT_CONNECTEDDEVICES - sizeof(CFastArray<uintptr_t>)];
	uint32_t IsFocused;
};

class CMwValueStd
{
public:
	void* pValue;
	void* pValue2;
	unsigned char Pad[16];
};

class CMwStack;

// C4100: unreferenced parameter for function
#pragma warning(disable : 4100)
#pragma warning(push)

// The base class for all game objects
class CMwNod
{
public:
#ifdef MANIAPLANET
	virtual ~CMwNod(void) {}
	virtual void VTablePadding1(void) {}
	virtual CMwClassInfo* MwGetClassInfo(void) { return nullptr; }
	virtual unsigned long MwGetMwClassId(void) { return 0L; }
	virtual bool MwIsKindOf(unsigned long classID) { return false; }
	virtual uint64_t* MwGetId(void) { return nullptr; }
	virtual void MwSetIdName(char const* psz) {}
	virtual void MwIsKilled(CMwNod*) {}
	virtual void MwIsUnreferenced(CMwNod*) {}
	virtual unsigned long VirtualParam_Get(CMwStack* pStack, void* ppValue) { return 0L; }
	virtual unsigned long VirtualParam_Set(CMwStack* pStack, void* pValue)  { return 0L; }
	virtual unsigned long VirtualParam_Add(CMwStack* pStack, void* pValue)  { return 0L; }
	virtual unsigned long VirtualParam_Sub(CMwStack* pStack, void* pValue)  { return 0L; }
#else
	virtual ~CMwNod(void) {}
	virtual CMwClassInfo* MwGetClassInfo(void) { return nullptr; }
	virtual unsigned long MwGetMwClassId(void) { return 0; }
	virtual bool MwIsKindOf(unsigned long classID) { return false; }
	virtual uint64_t* MwGetId(void) { return nullptr; }
	virtual void MwSetIdName(char const* psz) {}
	virtual void VTablePadding6(void) {}
	virtual void MwIsUnreferenced(CMwNod*) {}
	virtual unsigned long VirtualParam_Get(CMwStack* pStack, void* ppValue) { return 0L; }
	virtual unsigned long VirtualParam_Set(CMwStack* pStack, void* pValue)  { return 0L; }
	virtual unsigned long VirtualParam_Add(CMwStack* pStack, void* pValue)  { return 0L; }
	virtual unsigned long VirtualParam_Sub(CMwStack* pStack, void* pValue)  { return 0L; }
#endif

	uint32_t ReferenceCount;
	uint32_t Pad0;
	uintptr_t SystemFid;
	CFastBuffer<CMwNod*>* Dependants;
	uint32_t unknown1;
	uint32_t Pad1;
};

class CMwStack
{
public:
#ifdef GAMEBOX
	// Type of an item in the game's stack. The game keeps the types in an array next to the items
	enum ItemType
	{
		ITEM_MEMBER = 0,
		ITEM_BOOL = 0x10000001,
		ITEM_OBJECT = 0x10000002,
		ITEM_ENUM = 0x10000003,
		ITEM_ISO4 = 0x10000004,
		ITEM_VEC2 = 0x10000005,
		ITEM_VEC3 = 0x10000006,
		ITEM_INT = 0x10000007,
		ITEM_UINT = 0x10000008,
		ITEM_FLOAT = 0x10000009,
		ITEM_STRING = 0x1000000A
	};

	struct Item
	{
		void* Value;
		ItemType Type;
	};

	CMwStack()
	{
		// The game's stack is a nod, so give it a vtable whose methods do nothing
		static CMwNod VtableSource;
		Vtable = *(void**)&VtableSource;
	}

	CMwStack(const CMwStack&) = delete;
	CMwStack& operator=(const CMwStack&) = delete;

	~CMwStack()
	{
		free(Items);
		free(Types);
	}

	// CMwNod's fields. CMwNod in this file is bigger than the game's, so the stack can't inherit from it
	void* Vtable = nullptr;
	uint32_t NodFields[4] = {};

	uint32_t Size = 0;
	uint32_t Capacity = 0;
	// Allocated with malloc, freed by the destructor
	void** Items = nullptr;
	// One type per item
	ItemType* Types = nullptr;
	// Index of the item the game reads next, counting down
	int32_t CurrentPos = 0;
#elif defined(TMCN)
	enum ItemType
	{
		ITEM_MEMBER = 0,
		ITEM_BOOL = 0x10000001,
		ITEM_OBJECT = 0x10000002,
		ITEM_ENUM = 0x10000003,
		ITEM_ISO4 = 0x10000004,
		ITEM_VEC2 = 0x10000005,
		ITEM_VEC3 = 0x10000006,
		ITEM_INT2 = 0x10000007,
		ITEM_INT3 = 0x10000008,
		ITEM_UINT3 = 0x10000009,
		ITEM_INT = 0x1000000A,
		ITEM_UINT = 0x1000000B,
		ITEM_FLOAT = 0x1000000C,
		ITEM_STRING = 0x1000000D,
		ITEM_WSTRING = 0x1000000E
	};

	struct Item
	{
		void* Value;
		ItemType Type;
	};

	// The game reads these three as single bytes
	uint8_t  CurrentPos;
	uint8_t  Size;
	uint8_t  ExtraItemsCapacity;
	uint8_t  Pad[5];
	Item     ContainedItems[2];
	// Items from index 2 onwards
	Item*    ExtraItems;
#endif
};

class CMwEngineInfo
{
public:
	virtual ~CMwEngineInfo() {}

	uint32_t EngineID;
	char* EngineName;
#ifdef GAMEBOX
	uint32_t Padding;
#endif
	uint32_t ClassesAmount; // ?? Classes already has this info
#ifdef TMCN
	CFastArray<CMwClassInfo*> Classes;
#else
	CMwClassInfo** Classes;
#endif
};

class CMwEngineManager
{
public:
	virtual ~CMwEngineManager() {}

#ifdef GAMEBOX
	uint32_t Padding;
	uint32_t Padding2;
#endif

	CFastArray<CMwEngineInfo*> Engines;
};

template <typename WantedType>
consteval
CMwStack::ItemType TypeToStackItemType()
{
	if constexpr (std::is_same_v<WantedType, bool>)
	{
		return CMwStack::ITEM_BOOL;
	}
	else if constexpr (std::is_same_v<WantedType, CMwNod*>)
	{
		return CMwStack::ITEM_OBJECT;
	}
	else if constexpr (std::is_enum_v<WantedType>)
	{
		return CMwStack::ITEM_ENUM;
	}
	else if constexpr (std::is_same_v<WantedType, int32_t>)
	{
		return CMwStack::ITEM_INT;
	}
	else if constexpr (std::is_same_v<WantedType, uint32_t>)
	{
		return CMwStack::ITEM_UINT;
	}
	else if constexpr (std::is_same_v<WantedType, float>)
	{
		return CMwStack::ITEM_FLOAT;
	}
	else if constexpr (std::is_same_v<WantedType, Vec2>)
	{
		return CMwStack::ITEM_VEC2;
	}
	else if constexpr (std::is_same_v<WantedType, Vec3>)
	{
		return CMwStack::ITEM_VEC3;
	}
	else if constexpr (std::is_same_v<WantedType, Iso4>)
	{
		return CMwStack::ITEM_ISO4;
	}
	else if constexpr (std::is_same_v<WantedType, CFastString> or std::is_same_v<WantedType, std::string>)
	{
		return CMwStack::ITEM_STRING;
	}
#ifdef TMCN
	else if constexpr (std::is_same_v<WantedType, CFastStringInt> or std::is_same_v<WantedType, std::wstring>)
	{
		return CMwStack::ITEM_WSTRING;
	}
#else
	else if constexpr (std::is_same_v<WantedType, CFastStringInt> or std::is_same_v<WantedType, std::wstring>)
	{
		static_assert(false, "wstring is not a valid argument in GAMEBOX.");
	}
#endif
	else
	{
		static_assert(false, "Must provide a valid and implemented type for TypeToStackItemType");
	}
}

#pragma warning(pop) // C4100