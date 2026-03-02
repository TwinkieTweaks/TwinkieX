#pragma once

// Modules
#include <Modules/ExampleModule/ExampleModule.h>
#include <Modules/AnotherModule/AnotherModule.h>

class TwinkUi;

#ifdef MANIAPLANET
using DirectXDevice = ID3D11Device;
using DirectXContext = ID3D11DeviceContext;

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
#else
using DirectXDevice = IDirect3DDevice9;

using ResetFn = HRESULT(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
using PresentFn = long(__stdcall*)(LPDIRECT3DDEVICE9 pDevice, LPVOID, LPVOID, HWND, LPVOID);

static long __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, LPVOID A, LPVOID B, HWND C, LPVOID D);

static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

// The namespace that holds all globals for the TwinkUiState
namespace TwinkUiState
{
	// The original present function definition for the device.
	extern PresentFn oPresent;

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

#ifdef MANIAPLANET
	// The DirectX11 context.
	extern DirectXContext* Context;
#endif
}

// The UI manager type, used by (TwinkUi gTwinkie.UiMgr).
class TwinkUi
{
public:
	// The Trackmania manager, set by (class Twinkie) to (TrackmaniaMgr Twinkie::TrackmaniaMgr)
	TwinkTrackmania* TrackmaniaMgr = nullptr;

	std::vector<IModule*> Modules = {};

	// Behaviors

	// Ctor
	TwinkUi(TwinkTrackmania& TrackmaniaMgr);
	// Dtor
	~TwinkUi() = default;

	// Methods
	// imgui render calls are allowed here
	void Render();

	// Updates the internal hooks to be up-to-date
	void Update();
};

#ifdef GAMEBOX
// Initialize dear ImGui
void InitImGui(DirectXDevice* Device);
#else
// Initialize dear ImGui
void InitImGui(DirectXContext* Context, DirectXDevice* Device);
#endif