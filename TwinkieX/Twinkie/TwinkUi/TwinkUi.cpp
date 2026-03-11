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

	// Draw the UI
	bool RenderUi = true;

	// The device, since it's not given by any function
	DirectXDevice* Device = 0;

	// The UiMgr
	TwinkUi* UiMgr = nullptr;

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

	this->Modules = {
		new AppExplorerModule(TrackmaniaMgr),
		new AnotherModule(TrackmaniaMgr)
	};
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
	Unprotect(VirtualPtr(O_V_PRESENT, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain()), sizeof(uintptr_t));

	// Hook present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(O_V_PRESENT, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain(), (uintptr_t)hkPresent));

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
	Unprotect(VirtualPtr(O_V_PRESENT, (uintptr_t)TwinkUiState::Device), sizeof(uintptr_t));

	// Hook present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(O_V_PRESENT, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkPresent));
#endif
}

static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!TwinkUiState::ImGuiInit)
	{
		return CallWindowProcA(TwinkUiState::oWndProc, hWnd, uMsg, wParam, lParam);
	}

	// TODO: Document
	auto& ImIo = ImGui::GetIO();

	auto ImWndProcResult = ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	if (ImWndProcResult)
	{
		return ImWndProcResult;
	}
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_F3 and !(lParam & 0xFF000000))
			TwinkUiState::RenderUi = !TwinkUiState::RenderUi;
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

	TwinkUiState::UiMgr->Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return TwinkUiState::oPresent(pDevice, A, B, C, D);
}
#elif defined(MANIAPLANET)
void InitImGui(DirectXContext* Context, DirectXDevice* Device)
{
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(TwinkUiState::Window);
	ImGui_ImplDX11_Init(Device, Context);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	// TODO: Move this to TwinkUiState
	static ID3D11RenderTargetView* mainRenderTargetView = nullptr;

	// If we haven't initialized ImGui yet...
	if (!TwinkUiState::ImGuiInit)
	{
		// ...then try to get the DX11 device
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&TwinkUiState::Device)))
		{
			// Get the context from the device
			TwinkUiState::Device->GetImmediateContext(&TwinkUiState::Context);

			// Get the swap chain description,
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);

			// so we can get the window out of it
			TwinkUiState::Window = sd.OutputWindow;

			// Get the 2D back buffer of the swap chain
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

			// Get the color format from the swap chain's back buffer for use with the render target view
			DXGI_FORMAT BackBufferColorFmt;
			{
				D3D11_TEXTURE2D_DESC desc;
				pBackBuffer->GetDesc(&desc);
				BackBufferColorFmt = desc.Format;
			}

			// Use the new render target view description with the new color format
			D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
			RenderTargetViewDesc.Format = BackBufferColorFmt;
			RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			// Create a target view using the buffer we got to render our UI on
			TwinkUiState::Device->CreateRenderTargetView(pBackBuffer, &RenderTargetViewDesc, &mainRenderTargetView);

			// Release the buffer so that the game can use it later
			pBackBuffer->Release();

			// Hook the window process
			TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			// Initialize ImGui
			InitImGui(TwinkUiState::Context, TwinkUiState::Device);
			TwinkUiState::ImGuiInit = true;
		}

		// If we couldn't get the device, then fallback to original behavior
		else
		{
			return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
		}
	}

	// Create a new frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	TwinkUiState::UiMgr->Render();

	ImGui::Render();

	// Set the render target for the draw data rendering
	TwinkUiState::Context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);

	// Draw from the draw data
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Continue as if nothing happened
	return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
}
#endif

void TwinkUi::Render()
{
	using namespace ImGui;

	if (TwinkUiState::RenderUi)
	{
		if (BeginMainMenuBar())
		{
			PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32({ 1.f, 0.f, 1.f, 1.f }));
			PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
			if (BeginMenu("Twinkie##Twinkie"))
			{
				PopStyleColor();

				ImGui::EndMenu();
			}
			else PopStyleColor();

			for (auto& Module : this->Modules)
			{
				Module->RenderMenuMain();
			}

			if (BeginMenu("Modules##Twinkie"))
			{
				for (auto& Module : this->Modules)
				{
					Module->RenderMenu();
				}

				ImGui::EndMenu();
			}

			PopItemFlag();

			EndMainMenuBar();
		}
	}

	for (auto& Module : this->Modules)
	{
		Module->Render();
		if (TwinkUiState::RenderUi) Module->RenderInterface();
	}
}

// END Methods