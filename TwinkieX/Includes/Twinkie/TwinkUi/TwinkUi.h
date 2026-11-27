#pragma once

#include <Twinkie/TwinkLogs/TwinkLogs.h>
#include <Twinkie/TwinkDiscordRP/TwinkDiscordRP.h>
#include <Twinkie/TwinkIo/TwinkIo.h>
#include <Twinkie/TwinkFilePicker/TwinkFilePickerExports.h>

#include <Modules/IModule.h>

#include <vector>

class TwinkUi;

#ifdef MANIAPLANET
using DirectXDevice = ID3D11Device;
using DirectXContext = ID3D11DeviceContext;

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT hkResizeBuffers(IDXGISwapChain* pThis, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

#else
using DirectXDevice = IDirect3DDevice9;

using PresentFn = long(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, LPVOID, LPVOID, HWND, LPVOID);
using ResetFn = long(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pParams);

static long __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, LPVOID A, LPVOID B, HWND C, LPVOID D);
#ifdef O_V_RESET
static long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pParams);
#endif

static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

// The namespace that holds all globals for the TwinkUiState
namespace TwinkUiState
{
	// The original present function definition for the device.
	extern PresentFn oPresent;

#ifdef O_V_RESET
	// The original reset function definition for the device.
	extern ResetFn oReset;
#endif

	// The original window process definition for the window.
	extern WNDPROC oWndProc;

	// Handle to the game window.
	extern HWND Window;

	// Check if dear ImGui has been initialized yet
	extern bool ImGuiInit;

	// Draw the UI
	extern bool RenderUi;

	// The device, since it's not given by any function
	extern DirectXDevice* Device;

	// The UiMgr
	extern TwinkUi* UiMgr;

	// The flags every Twinkie and Gecko window starts from, from the Overlay settings
	int DefaultWindowFlags();

	// Whether the process is exiting, in which case teardown must be skipped
	extern bool ProcessTerminating;

#ifdef MANIAPLANET
	// The DirectX11 context.
	extern DirectXContext* Context;

	// The main render target view.
	extern ID3D11RenderTargetView* MainRenderTargetView;

	// The original resize buffers function for this DX11 device;
	extern ResizeBuffersFn oResizeBuffers;
#elif defined(GAMEBOX)
	// The new window size, when the window is resized.
	extern unsigned int WindowWidth;
	// The new window size, when the window is resized.
	extern unsigned int WindowHeight;
#endif
}

// The UI manager type, used by (TwinkUi gTwinkie.UiMgr).
class TwinkUi
{
	TwinkUi operator=(TwinkUi) = delete;
	TwinkUi& operator=(TwinkUi&) = delete;
	TwinkUi&& operator=(TwinkUi&&) = delete;
public:
	// The Trackmania manager, set by (class Twinkie) to (TrackmaniaMgr Twinkie::TrackmaniaMgr)
	TwinkTrackmania* TrackmaniaMgr = nullptr;

	// The IO manager
	TwinkIo IoMgr;

	// The Discord Rich Presence manager, managed by this class
	TwinkDiscordRP* DiscordMgr = nullptr;

	std::vector<IModule*> Modules = {};

	// Used when another thread wants to add a module
	std::vector<IModule*> ModuleQueue = {};

	// The current font path.
	std::u8string FontPath = u8"";

	// The Documents folder path.
	std::filesystem::path DocumentsFolderPath;

	// The current font itself.
	ImFont* Font = nullptr;

	// The file picker.
	TwinkFilePicker* FilePicker = nullptr;

	// The size of the font.
	float UiScale = 1.f;

	// Settings window
	bool ShowSettings = false;
	bool SettingsWereShown = false;

	// True if the current font is set to be destroyed/removed.
	bool DestroyCurrentFont = false;

	// Overlay settings for UI::GetDefaultWindowFlags()
	bool EnableWindowCollapsing = false;
	bool EnableWindowDocking = false;
	bool EnableWindowDockingWithShift = false;

	int GetDefaultWindowFlags() const;

	TwinkLogs Logger;

	// Behaviors

	// Ctor
	TwinkUi(TwinkTrackmania& TrackmaniaMgr);
	// Dtor
	~TwinkUi();

	// Methods
	// imgui render calls are allowed here
	void Render();

	// Updates the internal hooks to be up-to-date
	void Update(std::filesystem::path DocumentsFolderPath);

	// Adds a font from a file, with the icons merged into it
	ImFont* AddFont(const char* Path);

	// Merges the icons Gecko's Icons:: strings use (Fonts\ManiaIcons.ttf) into the font added last
	void MergeIcons();
};

#ifdef GAMEBOX
// Initialize dear ImGui
void InitImGui(DirectXDevice* Device);
#else
// Initialize dear ImGui
void InitImGui(DirectXContext* Context, DirectXDevice* Device);
#endif