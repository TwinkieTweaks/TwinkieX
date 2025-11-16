// Precompiled headers.
#include "pch.h"

// External dear ImGui WndProc handler declaration
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Class definition for the TwinkTrackmania class.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Class definition for the TwinkUi class.
#include <Twinkie/TwinkUi/TwinkUi.h>

// Namespace

namespace TwinkUiState
{
	// The original present function definition for the device.
	PresentFn oPresent = 0;

	// The original window process definition for the window.
	WNDPROC oWndProc = 0;

	// Handle to the game window.
	HWND Window = 0;

	// Check if dear ImGui has been initialized yet
	bool ImGuiInit = false;

	// The device, since it's not given by any function
	DirectXDevice* Device = 0;

#ifdef MANIAPLANET
	// The DirectX11 context.
	DirectXContext* Context;
#endif
}

// END Namespace

// Behaviors

__declspec(noinline) TwinkUi::TwinkUi(TwinkTrackmania& TrackmaniaMgr)
{
	// Set the trackmania manager reference for later use
	this->TrackmaniaMgr = &TrackmaniaMgr;
}

// END Behaviors

// Methods

__declspec(noinline) void TwinkUi::Update()
{
	// Get the device from the game
	TwinkUiState::Device = (DirectXDevice*)this->TrackmaniaMgr->GetDirectXDevice();

	// If the device is not initialized yet,
	if (!TwinkUiState::Device)
	{
		// Quit
		return;
		// This will quit only when the constructor (TwinkUi::TwinkUi) is called because it is called when initializing the (TwinkUi Twinkie::UiMgr).
	}

	// TMCN
#ifdef MANIAPLANET
	// Get the context for later use with dear ImGui
	TwinkUiState::Device->GetImmediateContext(&TwinkUiState::Context);

	// The window is later gotten in the present hook itself, so we don't need to implement it

	// Before hooking, make sure that our memory is writable
	Unprotect(VirtualPtr(8, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain()), sizeof(uintptr_t));

	// Hook present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(8, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain(), (uintptr_t)hkPresent));

	// Everything else
#else

	// Create a creation parameters struct to hold the result
	D3DDEVICE_CREATION_PARAMETERS D3DCreationParams = {};

	// While (      we could not get the creation parameters successfully      ) or (the parameters have no window,)
	while (  (TwinkUiState::Device->GetCreationParameters(&D3DCreationParams) != D3D_OK) or !D3DCreationParams.hFocusWindow)
	{
		// Wait
		Sleep(1);
	}

	// Get the window from the creation parameters
	TwinkUiState::Window = D3DCreationParams.hFocusWindow;

	// Set our new window process
	TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

	// Before hooking, make sure that our memory is writable
	Unprotect(VirtualPtr(17, (uintptr_t)TwinkUiState::Device), sizeof(uintptr_t));

	// Hook present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(17, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkPresent));
#endif
}

static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto& ImIo = ImGui::GetIO();

	auto ImWndProcResult = ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	if (ImWndProcResult)
	{
		return ImWndProcResult;
	}

	if ((uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST && ImIo.WantCaptureKeyboard) || (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && ImIo.WantCaptureMouse))
	{
		return 1;
	}

	return CallWindowProcA(TwinkUiState::oWndProc, hWnd, uMsg, wParam, lParam);
}

#ifdef GAMEBOX
void InitImGui(DirectXDevice* Device)
{
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(TwinkUiState::Window);
	ImGui_ImplDX9_Init(Device);
}

static long __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, LPVOID A, LPVOID B, HWND C, LPVOID D)
{
	if (!TwinkUiState::ImGuiInit)
	{
		InitImGui(pDevice);
		TwinkUiState::ImGuiInit = true;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return TwinkUiState::oPresent(pDevice, A, B, C, D);
}
#else
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	// TODO: Move this to TwinkUiState

	static ID3D11RenderTargetView* mainRenderTargetView = nullptr;

	// 
	if (!TwinkUiState::ImGuiInit)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&TwinkUiState::Device)))
		{
			TwinkUiState::Device->GetImmediateContext(&TwinkUiState::Context);

			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);

			TwinkUiState::Window = sd.OutputWindow;

			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			TwinkUiState::Device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();

			TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			InitImGui(TwinkUiState::Context, TwinkUiState::Device);

			TwinkUiState::ImGuiInit = true;
		}

		else
		{
			return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
		}
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

	TwinkUiState::Context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
}

void InitImGui(DirectXContext* Context, DirectXDevice* Device)
{
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(TwinkUiState::Window);
	ImGui_ImplDX11_Init(Device, Context);
}
#endif

// END Methods