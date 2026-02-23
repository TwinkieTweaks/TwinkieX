#pragma once

typedef int Integer;
typedef unsigned int Bool;
typedef unsigned char Nat8;
typedef unsigned short Nat16;
typedef unsigned int Natural, Nat32;
typedef unsigned __int64 Nat64;
typedef float Real, Real32;

typedef void(*pFun)(void*);

// Resizable array type used by the game
template <typename T>
struct CFastBuffer
{
#ifdef TMCN
	T* Ptr = nullptr;
	size_t Size = 0;
#elif defined(GAMEBOX)
	size_t Size = 0;
	T* Ptr = nullptr;
#endif

	size_t Capacity = 0;

	T* operator[](size_t Idx)
	{
		return Ptr + Idx;
	}

	T* begin() { return Ptr; }
	T* end() { return Ptr + Size; }

	const T* begin() const { return Ptr; }
	const T* end() const { return Ptr + Size; }
};

// Used for parameters to PROCs
struct CMwParam 
{
	void* Ptr;
};

// The base class for all game objects
struct CMwNod 
{
public:
	pFun* vftable;
	Natural ReferenceCount;
	unsigned int Pad0;
	uintptr_t SystemFid;
	CFastBuffer<CMwNod*>* Dependants;
	Natural unknown1;
	unsigned int Pad1;
};

#ifdef TMCN
// Information about a class' member
// This class is exclusive to TMCN
struct CMwMemberInfo
{
	enum eType
	{
		ACTION,          // Method that takes no arguments and has no return value; CMwMethodInfo
		BOOL,
		BOOLARRAY,
		BOOLBUFFER,
		BOOLBUFFERCAT,
		CLASS,           // CMwClassMemberInfo
		CLASSARRAY,      // CMwClassArrayMemberInfo
		CLASSBUFFER,     // CMwClassArrayMemberInfo
		CLASSBUFFERCAT,  // CMwClassArrayMemberInfo
		COLOR,           // Color struct
		COLORARRAY,
		COLORBUFFER,
		COLORBUFFERCAT,
		ENUM,            // CMwEnumInfo
		ENUMARRAY,
		ENUMBUFFER,
		ENUMBUFFERCAT,
		INT,
		INTARRAY,
		INTBUFFER,
		INTBUFFERCAT,
		INTRANGE,
		ISO4,            // Iso4
		ISO4ARRAY,
		ISO4BUFFER,
		ISO4BUFFERCAT,
		ISO3,            // Iso3
		ISO3ARRAY,
		ISO3BUFFER,
		ISO3BUFFERCAT,
		ID,              // Id
		IDARRAY,
		IDBUFFER,
		IDBUFFERCAT,
		NATURAL,         // unsigned int
		NATURALARRAY,
		NATURALBUFFER,
		NATURALBUFFERCAT,
		NATURALRANGE,
		REAL,            // float
		REALARRAY,
		REALBUFFER,
		REALBUFFERCAT,
		REALRANGE,
		STRING,          // String
		STRINGARRAY,
		STRINGBUFFER,
		STRINGBUFFERCAT,
		STRINGINT,       // StringInt
		STRINGINTARRAY,
		STRINGINTBUFFER,
		STRINGINTBUFFERCAT,
		VEC2,            // Vec2
		VEC2ARRAY,
		VEC2BUFFER,
		VEC2BUFFERCAT,
		VEC3,            // Vec3
		VEC3ARRAY,
		VEC3BUFFER,
		VEC3BUFFERCAT,
		VEC4,            // Vec4
		VEC4ARRAY,
		VEC4BUFFER,
		VEC4BUFFERCAT,
		INT3,            // Int3
		INT3ARRAY,
		INT3BUFFER,
		INT3BUFFERCAT,
		NAT3,            // Nat3
		NAT3ARRAY,
		NAT3BUFFER,
		NAT3BUFFERCAT,
		QUAT,            // Quaternion
		QUATARRAY,
		QUATBUFFER,
		QUATBUFFERCAT,
		PROC             // Method with arguments and/or a return value; CMwMethodInfo
	};

	enum eFlags 
	{
		READ = 0b00000001,
		WRITE = 0b00000010,
		U1 = 0b00000100,
		U2 = 0b00001000,
		VIRTUAL_GET = 0b00010000,
		VIRTUAL_SET = 0b00100000,
		VIRTUAL_ADD = 0b01000000,
		VIRTUAL_SUB = 0b10000000,
	};

	// Type of the member
	eType MemberType;

	// Member ID, unique between classes
	int MemberID;

	// I have no idea
	CMwParam* pParam;

	// Offset of the member in the class instance, -1 for methods and virtual members
	int MemberOffset;

	int Pad0;

	// Name of the member
	const char* MemberName;

	// Unknown pointer
	void* p;

	// Flags for the member, see (eFlags)
	int flags;
	// Secondary flags for the member, see (eFlags)
	int flags2;
};


// Information about a class
// This class is exclusive to TMCN
struct CMwClassInfo
{
	// Virtual function table pointer
	pFun* vftable;

	// Class name
	char* ClassName;

	int unknown0;
	int unknown1;

	// Class ID
	int ClassID;

	int unknown2;

	// Parent class' info, null if none
	CMwClassInfo* ParentClassInfo;

	// Next class info, usually has nothing to do with this class
	CMwClassInfo* NextClassInfo;
	// Pointer to the class' constructor
	pFun CtorFn;

	// Padding for unknown data
	unsigned long long Padding[18];

	// File extension for this class
	char* FileExtName;

	// List of member infos
	CMwMemberInfo** Members;
	// Number of member infos in the list
	unsigned int MembersAmount;

	// Iterator function for the beginning of the members array
	CMwMemberInfo** begin();
	// Iterator function for the end of the members array
	CMwMemberInfo** end();

	// Const iterator function for the beginning of the members array
	CMwMemberInfo** begin() const;
	// Const iterator function for the end of the members array
	CMwMemberInfo** end() const;

	int Pad0;
};
// GAMEBOX is for TM1, TMO, TMS, TMSX and ESWC
#elif defined(GAMEBOX)
// Information about a class' member
// This class is shared between TM1, TMO, TMS, TMSX and ESWC
struct CMwMemberInfo
{
	enum eType
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
		ID,
		IDARRAY,
		IDBUFFER,
		IDBUFFERCAT,
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
		STRINGINT,
		STRINGINTARRAY,
		STRINGINTBUFFER,
		STRINGINTBUFFERCAT,
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
		INT3,
		INT3ARRAY,
		INT3BUFFER,
		INT3BUFFERCAT,
		PROC
	};

	enum eFlags
	{
		READ = 0b00000001,
		WRITE = 0b00000010,
		U1 = 0b00000100,
		U2 = 0b00001000,
		VIRTUAL_GET = 0b00010000,
		VIRTUAL_SET = 0b00100000,
		VIRTUAL_ADD = 0b01000000,
		VIRTUAL_SUB = 0b10000000,
	};

	// Type of the member
	eType MemberType;

	// Member ID, unique between classes
	int MemberID;

	// I have no idea
	CMwParam* pParam;

	// Offset of the member in the class instance, -1 for methods and virtual members
	int MemberOffset;

	// Member name
	const char* MemberName;

	// Flags for the member, see (eFlags)
	int flags;
	// Secondary flags for the member, see (eFlags)
	int flags2;
};

// Information about a class' member
// This class is shared between TM1, TMO, TMS, TMSX and ESWC
struct CMwClassInfo
{
	// Virtual function table pointer
	pFun* vftable;

	// Class ID
	int ClassID;

	// Parent class' info, null if none
	CMwClassInfo* ParentClassInfo;

	int unknown0;
	int unknown1;
	
	// Class name
	char* ClassName;

	// Next class info, usually has nothing to do with this class
	CMwClassInfo* NextClassInfo;
	// Pointer to the class' constructor
	pFun CtorFn;

	// List of member infos
	CMwMemberInfo** Members;
	// Number of member infos in the list
	unsigned int MembersAmount;

	// Iterator function for the beginning of the members array
	CMwMemberInfo** begin();
	// Iterator function for the end of the members array
	CMwMemberInfo** end();

	// Const iterator function for the beginning of the members array
	CMwMemberInfo** begin() const;
	// Const iterator function for the end of the members array
	CMwMemberInfo** end() const;
};
#endif
