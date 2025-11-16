// Precompiled headers.
#include "pch.h"

#include <Utils/Offsets.h>

DWORD ProtFlags;

uintptr_t VirtualWrite(unsigned int Idx, uintptr_t This, uintptr_t ToWrite)
{
    uintptr_t* Vtable = *reinterpret_cast<uintptr_t**>(This);

    auto PrevValue = Vtable[Idx];
    Vtable[Idx] = ToWrite;

    return PrevValue;
}