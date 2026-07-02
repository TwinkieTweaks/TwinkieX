// Precompiled headers.
#include "pch.h"

// imgui (just for intellisense)
#ifdef GAMEBOX
#include <imgui/imgui_impl_dx9.h>
#else
#include <imgui/imgui_impl_dx11.h>
#endif

#include <imgui/imgui_impl_win32.h>

// External dear ImGui WndProc handler declaration
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Class definition for the TwinkTrackmania class.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Class definition for the TwinkUi class, along with module headers.
#include <Twinkie/TwinkUi/TwinkUi.h>

// Discord
#include <DiscordGameSDK/DiscordStuff.h>

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

	// The main render target view.
	ID3D11RenderTargetView* MainRenderTargetView = nullptr;

	ResizeBuffersFn oResizeBuffers = nullptr;
#else defined(GAMEBOX)
	// The new window size, when the window is resized.
	unsigned int WindowWidth = 0;
	// The new window size, when the window is resized.
	unsigned int WindowHeight = 0;
#endif
}

// END Namespace

// Behaviors

__declspec(noinline) TwinkUi::TwinkUi(TwinkTrackmania& TrackmaniaMgr)
{
	// Set the trackmania manager reference for later use
	this->TrackmaniaMgr = &TrackmaniaMgr;

	this->DiscordMgr = new TwinkDiscordRP(TrackmaniaMgr);

	this->Modules.push_back(new AppExplorerModule(TrackmaniaMgr));
#ifdef TMCN
	this->Modules.push_back(new AddictionUnlimiter(TrackmaniaMgr));
	this->Modules.push_back(new VehicleExplorer(TrackmaniaMgr));
#endif
}

__declspec(noinline) TwinkUi::~TwinkUi()
{
	for (auto& Module : Modules)
	{
		delete Module;
	}
	for (auto& Module : ModuleQueue)
	{
		delete Module;
	}

	delete DiscordMgr;
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
	Unprotect(VirtualPtr(O_V_RESIZEBUFFERS, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain()), sizeof(uintptr_t));

	// Hook present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(O_V_PRESENT, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain(), (uintptr_t)hkPresent));

	// Hook ResizeBuffers
	TwinkUiState::oResizeBuffers = reinterpret_cast<ResizeBuffersFn>(VirtualWrite(O_V_RESIZEBUFFERS, (uintptr_t)this->TrackmaniaMgr->GetDirectXSwapChain(), (uintptr_t)hkResizeBuffers));

	// Everything else
#else

	// Create a creation parameters struct to hold the result
	D3DDEVICE_CREATION_PARAMETERS D3DCreationParams = {};

	// While (          we could not get the creation parameters successfully          ) or (the parameters have no window,)
	while ((TwinkUiState::Device->GetCreationParameters(&D3DCreationParams) != D3D_OK) or !D3DCreationParams.hFocusWindow)
	{
		// Wait
		Sleep(1);
	}

	// Get the window from the creation parameters
	TwinkUiState::Window = D3DCreationParams.hFocusWindow;

	// Set our new window process
	TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	unsigned int WindowStyle = GetWindowLongPtr(TwinkUiState::Window, GWL_STYLE);
	WindowStyle |= WS_SIZEBOX | WS_THICKFRAME;
	SetWindowLongPtr(TwinkUiState::Window, GWL_STYLE, WindowStyle);

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

	switch (uMsg)
	{
#ifdef GAMEBOX
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			TwinkUiState::WindowWidth = LOWORD(lParam);
			TwinkUiState::WindowHeight = HIWORD(lParam);
			ImGui::GetIO().DisplaySize = ImVec2((float)LOWORD(lParam), (float)HIWORD(lParam));

			// Manually trigger a repaint even if shrinking
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	case WM_PAINT:
		// Only trigger if we are actually initialized
		if (TwinkUiState::ImGuiInit)
		{
			// Get the implicit swap chain (index 0)
			IDirect3DSwapChain9* pSwapChain = nullptr;
			if (SUCCEEDED(TwinkUiState::Device->GetSwapChain(0, &pSwapChain)))
			{
				D3DPRESENT_PARAMETERS d3dpp = {};
				// Populate d3dpp with current settings
				if (SUCCEEDED(pSwapChain->GetPresentParameters(&d3dpp)))
				{
					// Update only the dimensions
					d3dpp.BackBufferWidth = TwinkUiState::WindowWidth;
					d3dpp.BackBufferHeight = TwinkUiState::WindowHeight;

					// Carry out the Reset
					ImGui_ImplDX9_InvalidateDeviceObjects();

					HRESULT hr = TwinkUiState::Device->Reset(&d3dpp);
					if (SUCCEEDED(hr))
					{
						ImGui_ImplDX9_CreateDeviceObjects();
					}

					unsigned int WindowStyle = GetWindowLongPtr(TwinkUiState::Window, GWL_STYLE);
					WindowStyle |= WS_SIZEBOX | WS_THICKFRAME;
					SetWindowLongPtr(TwinkUiState::Window, GWL_STYLE, WindowStyle);
				}
				pSwapChain->Release(); // Don't forget to release the COM object
			}
		}
		break;
#endif
	case WM_FILE_SELECTED:
	{
		// Empty on purpose (for now)
		break;
	}
	}

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

	IDirect3DStateBlock9* pStateBlock = NULL;
	if (pDevice->CreateStateBlock(D3DSBT_ALL, &pStateBlock) == D3D_OK)
	{
		pStateBlock->Capture();
		pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (not TwinkUiState::ImGuiInit)
		{
			if (not TwinkUiState::UiMgr->FontPath.empty())
			{
				TwinkUiState::UiMgr->Font = ImGui::GetIO().Fonts->AddFontFromFileTTF((char*)TwinkUiState::UiMgr->FontPath.c_str());
			}
			TwinkUiState::ImGuiInit = true;
		}

		TwinkUiState::UiMgr->Render();

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		if (TwinkUiState::UiMgr->DestroyCurrentFont and TwinkUiState::UiMgr->Font)
		{
			TwinkUiState::UiMgr->FontPath = u8"";
			ImGui::GetIO().Fonts->RemoveFont(TwinkUiState::UiMgr->Font);
			TwinkUiState::UiMgr->Font = nullptr;
			TwinkUiState::UiMgr->DestroyCurrentFont = false;
		}

		pStateBlock->Apply();
		pStateBlock->Release();
	}

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

			// Remove sRGB from the backbuffer color format, since imgui already uses sRGB
			if (BackBufferColorFmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
			{
				BackBufferColorFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			else if (BackBufferColorFmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
			{
				BackBufferColorFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			// Use the new render target view description with the new color format
			D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
			RenderTargetViewDesc.Format = BackBufferColorFmt;
			RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			// Create a target view using the buffer we got to render our UI on
			TwinkUiState::Device->CreateRenderTargetView(pBackBuffer, &RenderTargetViewDesc, &TwinkUiState::MainRenderTargetView);

			// Release the buffer so that the game can use it later
			pBackBuffer->Release();

			// Hook the window process
			TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			// Initialize ImGui
			InitImGui(TwinkUiState::Context, TwinkUiState::Device);
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
			
	if (not TwinkUiState::ImGuiInit)
	{
		if (not TwinkUiState::UiMgr->FontPath.empty())
		{
			TwinkUiState::UiMgr->Font = ImGui::GetIO().Fonts->AddFontFromFileTTF((char*)TwinkUiState::UiMgr->FontPath.c_str());
		}
		TwinkUiState::ImGuiInit = true;
	}

	TwinkUiState::UiMgr->Render();

	ImGui::Render();

	// Set the render target for the draw data rendering
	TwinkUiState::Context->OMSetRenderTargets(1, &TwinkUiState::MainRenderTargetView, NULL);

	// Draw from the draw data
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (TwinkUiState::UiMgr->DestroyCurrentFont and TwinkUiState::UiMgr->Font)
	{
		TwinkUiState::UiMgr->FontPath = u8"";
		ImGui::GetIO().Fonts->RemoveFont(TwinkUiState::UiMgr->Font);
		TwinkUiState::UiMgr->Font = nullptr;
		TwinkUiState::UiMgr->DestroyCurrentFont = false;
	}

	// Continue as if nothing happened
	return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
}

// https://github.com/GHFear/Universal-ImGui-D3D11-Hook-WithResize/blob/8bcad6865e4e579ab841ebe9ff16be09000251c4/DX11HookWithResize_Kiero_ImGui/DX11.h
HRESULT hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (TwinkUiState::MainRenderTargetView)
	{
		TwinkUiState::Context->OMSetRenderTargets(0, 0, 0);
		TwinkUiState::MainRenderTargetView->Release();
		TwinkUiState::MainRenderTargetView = nullptr; // Good practice to null it out
	}

	HRESULT resizeBuffers = TwinkUiState::oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ID3D11Texture2D* pBuffer;

	HRESULT pSwapChainBuffer = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
	if (pSwapChainBuffer != S_OK)
	{
		// TODO: Perform error handling here!
	}

	// --- STRIP sRGB SUFFIX ---
	D3D11_TEXTURE2D_DESC desc;
	pBuffer->GetDesc(&desc);
	DXGI_FORMAT BackBufferColorFmt = desc.Format;

	if (BackBufferColorFmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
	{
		BackBufferColorFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (BackBufferColorFmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
	{
		BackBufferColorFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
	RenderTargetViewDesc.Format = BackBufferColorFmt;
	RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// -------------------------

	// Pass our explicit descriptor instead of NULL
	HRESULT pDeviceRenderTarget = TwinkUiState::Device->CreateRenderTargetView(pBuffer, &RenderTargetViewDesc, &TwinkUiState::MainRenderTargetView);
	if (pDeviceRenderTarget != S_OK)
	{
		// TODO: Perform error handling here!
	}

	// Release buffer
	pBuffer->Release();

	// Set Rendertarget
	TwinkUiState::Context->OMSetRenderTargets(1, &TwinkUiState::MainRenderTargetView, NULL);

	// Set up the viewport.
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)Width;   // Cast to FLOAT to satisfy the struct and avoid warnings
	vp.Height = (FLOAT)Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	TwinkUiState::Context->RSSetViewports(1, &vp);

	// Return resizeBuffers
	return resizeBuffers;
}
#endif

void TwinkUi::Render()
{
	using namespace ImGui;

	if (Font) PushFont(Font, GetStyle().FontSizeBase * UiScale);

	if (not ModuleQueue.empty())
	{
		for (auto& Module : this->ModuleQueue)
		{
			Modules.push_back(Module);
			Veridian::Register("Modules", Module->ID, Module->Name, Veridian::VSettingType::VBool, &Module->Enabled);
		}
		ModuleQueue = {};
	}

	if (TwinkUiState::RenderUi)
	{
		if (BeginMainMenuBar())
		{
			PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

			if (BeginMenu("$f0fTwinkie##Twinkie"))
			{
				if (MenuItem("Include module"))
				{
					if (FilePicker)
					{
						delete FilePicker;
						FilePicker = nullptr;
					}

					FilePicker = new TwinkFilePicker(TwinkFilePicker::FilePickerPurpose::PickModule);
				}
				if (MenuItem("Settings", "", ShowSettings))
				{
					ShowSettings = !ShowSettings;
				}
				if (BeginMenu("Modules"))
				{
					for (auto& Module : this->Modules)
					{
						if (MenuItem(Module->Name, "", Module->Enabled))
						{
							Module->Enabled = !Module->Enabled;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (BeginMenu("Modules##Twinkie"))
			{
				for (auto& Module : this->Modules)
				{
					if (!Module->Enabled) continue;

					Module->RenderMenu();
				}

				ImGui::EndMenu();
			}

			for (auto& Module : this->Modules)
			{
				if (!Module->Enabled) continue;

				Module->RenderMenuMain();
			}

			PopItemFlag();

			EndMainMenuBar();
		}

		if (ShowSettings)
		{
			if (Begin("Settings##Twinkie", &ShowSettings))
			{
				Veridian::RenderAll("Twinkie");
			}
			End();
		}
	}

	for (auto& Module : this->Modules)
	{
		if (!Module->Enabled) continue;

		Module->Render();
		if (TwinkUiState::RenderUi) Module->RenderInterface();
	}

	if (Font) PopFont();

	// ============== NO IMGUI DRAW CALLS PAST THIS POINT==============

	if (FilePicker)
	{
		if (FilePicker->Done)
		{
			using enum TwinkFilePicker::FilePickerPurpose;

			switch (FilePicker->Purpose)
			{
				case PickModule:
				{
					LoadLibrary(FilePicker->FinalPath.c_str());
					break;
				}
				case PickFont:
				{
					char8_t UTF8FontPath[1025];
#ifdef TMCN
#pragma warning(push)
#pragma warning(disable : 4267) // warning that fires when casting size_t to int in 64-bit
#endif
					WideCharToMultiByte(CP_UTF8, 0, FilePicker->FinalPath.c_str(), FilePicker->FinalPath.length(), (char*)UTF8FontPath, 1024, NULL, NULL);
#ifdef TMCN
#pragma warning(pop)
#endif
					Font = GetIO().Fonts->AddFontFromFileTTF((char*)UTF8FontPath);
					FontPath = UTF8FontPath;
					break;
				}
				case NoPurpose: break;
			}

			delete FilePicker;
			FilePicker = nullptr;
		}
	}

	DiscordMgr->Update();
}

// END Methods