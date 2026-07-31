// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the DINPUT8LOADER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// DINPUT8LOADER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef DINPUT8LOADER_EXPORTS
#define DINPUT8LOADER_API __declspec(dllexport)
#else
#define DINPUT8LOADER_API __declspec(dllimport)
#endif

// This class is exported from the dll
class DINPUT8LOADER_API CDInput8Loader {
public:
	CDInput8Loader(void);
	// TODO: add your methods here.
};

extern DINPUT8LOADER_API int nDInput8Loader;

DINPUT8LOADER_API int fnDInput8Loader(void);
