// DInput8Loader.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "DInput8Loader.h"


// This is an example of an exported variable
DINPUT8LOADER_API int nDInput8Loader=0;

// This is an example of an exported function.
DINPUT8LOADER_API int fnDInput8Loader(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CDInput8Loader::CDInput8Loader()
{
    return;
}
