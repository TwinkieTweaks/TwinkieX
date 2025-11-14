#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN  
// This warning always fires when including Windows.h for some reason
#pragma warning(disable : 5039)
// Windows Header Files
#include <windows.h>

// Architecture detection header file
#include "Utils/Arch.h"