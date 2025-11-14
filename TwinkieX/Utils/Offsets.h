#pragma once
#include "Arch.h"

// Useful definitions

// Used to read from an address
#define Read(type, addr) (*(type*)((addr)))

// Used to write to an address
#define Write(type, addr, value) *(type*)((addr)) = (value)

// Used to get the n-th virtual function from an object ptr
#define Virtual(n, obj) (*(uintptr_t**)(obj))[n]

// Offsets

#if defined(TMCN)

#	define O_APP 0x1C7AB90

#elif defined(TM1)

#	define O_APP 0x0

#elif defined(TMS)

#	define O_APP 0x0

#elif defined(TMSX)

#	define O_APP 0x0

#elif defined(TMO)

#	define O_APP 0x0

#elif defined(ESWC)

#	define O_APP 0x0

#endif