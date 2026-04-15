#include <Utils/Offsets.h>
#pragma once

using ActionFn = void(__thiscall*)(uintptr_t);

// Resizable array type used by the game
template <typename T>
struct CFastBuffer
{
#ifdef TMCN
	T* Ptr = nullptr;
	unsigned int Size = 0;
	unsigned int Capacity = 0;
#elif defined(GAMEBOX)
	unsigned int Size = 0;
	T* Ptr = nullptr;
	unsigned int Capacity = 0;
#endif

	T* operator[](size_t Idx)
	{
		return Ptr + Idx;
	}

	T* begin() { return Ptr; }
	T* end() { return Ptr + Size; }

	const T* begin() const { return Ptr; }
	const T* end() const { return Ptr + Size; }
};

// Single-size array type used by the game
template <typename T>
struct CFastArray
{
#ifdef MANIAPLANET
	T* Ptr = nullptr;
	unsigned int Size = 0;
#else
	uintptr_t vf;
	unsigned int Size = 0;
	T* Ptr = nullptr;
#endif


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
	ActionFn* vftable;
	unsigned int ReferenceCount;
	unsigned int Pad0;
	uintptr_t SystemFid;
	CFastBuffer<CMwNod*>* Dependants;
	unsigned int unknown1;
	unsigned int Pad1;
};

#ifdef TMCN
// Information about a class' member
// This class is exclusive to TMCN
struct CMwMemberInfo
{
	// ARRAYBASE => https://openplanet.dev/docs/api/global/MwSArray
	// [TYPE] and [TYPE]ALT are DIFFERENT. Do NOT ignore them.
	enum eType
	{
		ACTION = 0,

		BOOL = 1,
		BOOLARRAY = 2, // EXTRAPOLATED
		BOOLBUFFER = 3, // EXTRAPOLATED
		BOOLBUFFERCAT = 4, // EXTRAPOLATED
		BOOLARRAYBASE = 5, // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray

		CLASS = 10,
		CLASSARRAY = 11,
		CLASSBUFFER = 12,
		CLASSBUFFERCAT = 13, // EXTRAPOLATED
		CLASSARRAYBASE = 14, // https://openplanet.dev/docs/api/global/MwSArray

		COLOR = 19, // EXTRAPOLATED
		COLORARRAY = 20,
		COLORBUFFER = 21, // EXTRAPOLATED
		COLORBUFFERCAT = 22,
		COLORARRAYBASE = 23, // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray

		ENUM = 28,

		INT = 37,
		INTARRAY = 38,
		INTBUFFER = 39, // EXTRAPOLATED
		INTBUFFERCAT = 40, // EXTRAPOLATED
		INTARRAYBASE = 41, // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray

		INTALT = 46,

		ISO4 = 47,

		ID = 65,
		
		NATURAL = 74,
		NATURALARRAY = 75, // EXTRAPOLATED
		NATURALBUFFER = 76, // EXTRAPOLATED
		NATURALBUFFERCAT = 77, // EXTRAPOLATED
		NATURALARRAYBASE = 78, // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray
		
		REAL = 84,
		REALARRAY = 85,
		REALBUFFER = 86, // EXTRAPOLATED
		REALBUFFERCAT = 87, // EXTRAPOLATED
		REALARRAYBASE = 88, // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray
		
		REALALT = 93,

		STRING = 94,
		STRINGARRAY = 95, // EXTRAPOLATED
		STRINGBUFFER = 96,
		STRINGBUFFERCAT = 97, // EXTRAPOLATED
		STRINGARRAYBASE = 98,  // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray
		
		STRINGINT = 103,
		STRINGINTARRAY = 104,
		STRINGINTBUFFER = 105,
		STRINGINTBUFFERCAT = 106, // EXTRAPOLATED
		STRINGINTARRAYBASE = 107,  // EXTRAPOLATED, https://openplanet.dev/docs/api/global/MwSArray
		
		VEC2 = 112,
		VEC3 = 121,
		
		PROC = 184,
		
		CLASSNOTPERSISTENT = 186 // https://next.openplanet.dev/MetaNotPersistent
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

	// TODO: Fully reverse this enum type
	// Type of the member
	eType MemberType;

	// Member ID, unique between classes
	int MemberID;

	// I have no idea
	CMwParam* pParam;

	// Offset of the member in the class instance, -1 for methods and virtual members
	unsigned int MemberOffset;

	int Pad0;

	// Name of the member
	const char* MemberName;

	// Unknown pointer
	void* p;

	// Flags for the member, see (eFlags)
	int MemberFlags;
	// Secondary flags for the member, see (eFlags)
	int MemberFlags2;
};


// Information about a class
// This class is exclusive to TMCN
struct CMwClassInfo
{
	int Pad1;

	unsigned int Pad2;

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
	ActionFn CtorFn;

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
	int MemberFlags;
	// Secondary flags for the member, see (eFlags)
	int MemberFlags2;
};

// Information about a class' member
// This class is shared between TM1, TMO, TMS, TMSX and ESWC
struct CMwClassInfo
{
	// Virtual function table pointer
	ActionFn* vftable;

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
	ActionFn CtorFn;

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

// Info of an ACTION type member
struct CMwMemberInfoAction : CMwMemberInfo
{
	char Padding[8];

	// The action function itself
	ActionFn Action;
};

// Info of a CLASS type member
struct CMwMemberInfoClass : CMwMemberInfo
{

};

// The class used by the app's input port (CTrackMania.InputPort)
struct CInputPort
{
	char Padding[O_M_CINPUTPORT_CONNECTEDDEVICES];
	CFastArray<uintptr_t> ConnectedDevices;
};
