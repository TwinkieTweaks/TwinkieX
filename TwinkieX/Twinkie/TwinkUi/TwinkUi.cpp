// Precompiled headers.
#include "pch.h"

#include <Modules/ModuleAutoExport.h>

// External dear ImGui WndProc handler declaration
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Class definition for the TwinkTrackmania class.
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// Class definition for the TwinkUi class, along with module headers.
#include <Twinkie/TwinkUi/TwinkUi.h>

// Discord
#include <DiscordGameSDK/DiscordStuff.h>

// Gecko, the angelscript scripting engine
#include <Gecko/Gecko.h>
#include <Gecko/GameCallbacks.h>

#include <Twinkie/TwinkNvg/TwinkNvg.h>
#include <Gecko/Exports/imgui/Texture/Texture.h>

// Namespace

namespace TwinkUiState
{

	// The original present function definition for the device.
	PresentFn oPresent = 0;

#ifdef O_V_RESET
	ResetFn oReset = 0;
#endif

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

	// Whether the process is exiting, in which case teardown must be skipped
	bool ProcessTerminating = false;

#ifdef MANIAPLANET
	// The DirectX11 context.
	DirectXContext* Context;

	// The main render target view.
	ID3D11RenderTargetView* MainRenderTargetView = nullptr;

	ResizeBuffersFn oResizeBuffers = nullptr;
#elif defined(GAMEBOX)
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
	wchar_t* Buffer = nullptr;
	SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, (wchar_t**)&Buffer);

	if (not Buffer) throw;

	DocumentsFolderPath = Buffer;
	CoTaskMemFree(Buffer);

	Veridian::InitContext(DocumentsFolderPath / "TwinkieX\\Twinkie.ini");

	// Set the trackmania manager reference for later use
	this->TrackmaniaMgr = &TrackmaniaMgr;

	// TODO: Make a loggert for discord RP
	this->DiscordMgr = new TwinkDiscordRP(TrackmaniaMgr);
}

__declspec(noinline) TwinkUi::~TwinkUi()
{
	if (TwinkUiState::ProcessTerminating) return;

	Veridian::VastVeridian->PrepareForDestruction();

	for (auto& Module : Modules)
	{
		delete Module;
	}
	for (auto& Module : ModuleQueue)
	{
		delete Module;
	}

	delete DiscordMgr;

	if (FilePicker) delete FilePicker;
}

// END Behaviors

// Methods
void TwinkUi::Render()
{
	using namespace ImGui;

	ImGuiIO& Io = GetIO();
	if (EnableWindowDocking) Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	else Io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
	Io.ConfigDockingWithShift = EnableWindowDockingWithShift;

	Gecko::TheScriptManager->RunAll(Gecko::CallbackType::Main);
	Gecko::TheScriptManager->RunAll(Gecko::CallbackType::Update, [](asIScriptContext* Ctx)
	{
		Ctx->SetArgFloat(0, GetIO().DeltaTime * 1000.0f);
	});

	IoMgr.Sync();

	static double LastSaveTime = 0.0;
	if (GetTime() - LastSaveTime > 1.0)
	{
		Veridian::VastVeridian->Save();
		LastSaveTime = GetTime();
	}

	if (IsKeyPressed(ImGuiKey_F3, false))
	{
		TwinkUiState::RenderUi = !TwinkUiState::RenderUi;
	}

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

	Gecko::TheScriptManager->RunAll(Gecko::CallbackType::RenderEarly);

	if (TwinkUiState::RenderUi)
	{
		if (BeginMainMenuBar())
		{
			PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

			if (BeginMenu("\\$f0fTwinkie##Twinkie"))
			{
				if (MenuItem("Include module"))
				{
					if (FilePicker)
					{
						delete FilePicker;
						FilePicker = nullptr;
					}

					FilePicker = new TwinkFilePicker(TwinkFilePicker::FilePickerPurpose::PickModule, DocumentsFolderPath / "TwinkieX\\Fonts");
				}
				if (MenuItem("Settings", "", ShowSettings))
				{
					ShowSettings = !ShowSettings;
				}
				if (MenuItem("Logs", "", Logger.Visible))
				{
					Logger.Visible = !Logger.Visible;
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

			Gecko::TheScriptManager->RenderDevMenu();

			if (BeginMenu("Modules##Twinkie"))
			{
				for (auto& Module : this->Modules)
				{
					if (!Module->Enabled) continue;

					Module->RenderMenu();
				}

				Gecko::TheScriptManager->RunAll(Gecko::CallbackType::RenderMenu);

				ImGui::EndMenu();
			}

			for (auto& Module : this->Modules)
			{
				if (!Module->Enabled) continue;

				Module->RenderMenuMain();
			}

			Gecko::TheScriptManager->RunAll(Gecko::CallbackType::RenderMenuMain);

			Logger.Render();

			PopItemFlag();

			ImGui::EndMainMenuBar();
		}
	}

	if (Gecko::TheScriptManager->SettingsOpenRequested)
	{
		Gecko::TheScriptManager->SettingsOpenRequested = false;
		ShowSettings = true;
		SetNextWindowFocus();
	}

	if (ShowSettings)
	{
		if (Begin("Settings##Twinkie", &ShowSettings, TwinkUiState::DefaultWindowFlags()))
		{
			Veridian::RenderAll("Twinkie");
		}
		End();
	}

	if (SettingsWereShown && !ShowSettings) Gecko::TheScriptManager->SaveSettings();
	SettingsWereShown = ShowSettings;

	for (auto& Module : this->Modules)
	{
		if (!Module->Enabled) continue;

		Module->Render();
		if (TwinkUiState::RenderUi) Module->RenderInterface();
	}

	Gecko::TheScriptManager->RunAll(Gecko::CallbackType::Render);
	if (TwinkUiState::RenderUi) Gecko::TheScriptManager->RunAll(Gecko::CallbackType::RenderInterface);

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
					char8_t UTF8FontPath[1025]{};
#ifdef TMCN
#pragma warning(push)
#pragma warning(disable : 4267) // warning that fires when casting size_t to int in 64-bit
#endif
					WideCharToMultiByte(CP_UTF8, 0, FilePicker->FinalPath.c_str(), FilePicker->FinalPath.length(), (char*)UTF8FontPath, 1024, NULL, NULL);
#ifdef TMCN
#pragma warning(pop)
#endif
					Font = AddFont((char*)UTF8FontPath);
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


int TwinkUi::GetDefaultWindowFlags() const
{
	int Flags = ImGuiWindowFlags_None;
	if (not EnableWindowCollapsing) Flags |= ImGuiWindowFlags_NoCollapse;
	return Flags;
}

int TwinkUiState::DefaultWindowFlags()
{
	return UiMgr ? UiMgr->GetDefaultWindowFlags() : ImGuiWindowFlags_None;
}

ImFont* TwinkUi::AddFont(const char* Path)
{
	ImFont* NewFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(Path);
	if (NewFont) MergeIcons();
	return NewFont;
}

void TwinkUi::MergeIcons()
{
	std::filesystem::path IconsPath = DocumentsFolderPath / L"TwinkieX\\Fonts\\ManiaIcons.ttf";

	std::error_code Error;
	if (not std::filesystem::exists(IconsPath, Error)) return;

	ImFontConfig Config;
	Config.MergeMode = true;
	Config.PixelSnapH = true;

	// Dear ImGui opens files as UTF-8, and the Documents folder can have any characters in it
	ImGui::GetIO().Fonts->AddFontFromFileTTF((const char*)IconsPath.u8string().c_str(), 0.0f, &Config);
}

__declspec(noinline) void TwinkUi::Update(std::filesystem::path pDocumentsFolderPath)
{
	IoMgr.Update();

	this->DocumentsFolderPath = pDocumentsFolderPath;

	// Get the device from the game
	TwinkUiState::Device = (DirectXDevice*)this->TrackmaniaMgr->GetDirectXDevice();

	// If the device is not initialized yet,
	if (!TwinkUiState::Device)
	{
		// Quit
		return;
		// This will quit only when the constructor (TwinkUi::TwinkUi) is called because it is called when initializing the (TwinkUi Twinkie::UiMgr).
	}

	CreateAndExportModules(this->Modules, *this->TrackmaniaMgr, &LoggertCpp);
	LoggertTwink.verbose(LogFormat("Created {} modules", this->Modules.size()));

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

	LoggertTwink.print(LogFormat("Hooked Present and ResizeBuffers on the swap chain {:#x}", this->TrackmaniaMgr->GetDirectXSwapChain()));
	LoggertTwink.verbose(LogFormat("Original Present {:#x}, original ResizeBuffers {:#x}, device {:#x}", (uintptr_t)TwinkUiState::oPresent, (uintptr_t)TwinkUiState::oResizeBuffers, (uintptr_t)TwinkUiState::Device));

	// Everything else
#else

	// Only patch the vtable here. Calling into the device or changing the window style from this thread
	// can deadlock the game: d3d9 keeps its lock owned by this thread after a device call, and changing
	// the style waits for the game thread, which can be stuck in Present waiting for that lock.
	// The device and window setup happens in hkPresent instead, on the game thread.

	// Before hooking, make sure that our memory is writable
	Unprotect(VirtualPtr(O_V_PRESENT, (uintptr_t)TwinkUiState::Device), sizeof(uintptr_t));

	// Hook Present
	TwinkUiState::oPresent = reinterpret_cast<PresentFn>(VirtualWrite(O_V_PRESENT, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkPresent));
	LoggertTwink.print(LogFormat("Hooked Present on the D3D9 device {:#x}", (uintptr_t)TwinkUiState::Device));
	LoggertTwink.verbose(LogFormat("Original Present {:#x}", (uintptr_t)TwinkUiState::oPresent));

#ifdef O_V_RESET
	Unprotect(VirtualPtr(O_V_RESET, (uintptr_t)TwinkUiState::Device), sizeof(uintptr_t));

	// Hook Reset
	TwinkUiState::oReset = reinterpret_cast<ResetFn>(VirtualWrite(O_V_RESET, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkReset));
	LoggertTwink.verbose(LogFormat("Hooked Reset, original {:#x}", (uintptr_t)TwinkUiState::oReset));
#endif
#endif
}

#if defined(GAMEBOX) && !defined(TMU)
static bool GetPresentParameters(IDirect3DDevice9* Device, D3DPRESENT_PARAMETERS& Params)
{
	IDirect3DSwapChain9* SwapChain = nullptr;
	if (!Device or FAILED(Device->GetSwapChain(0, &SwapChain)) or !SwapChain) return false;

	bool Got = SUCCEEDED(SwapChain->GetPresentParameters(&Params));
	SwapChain->Release();
	return Got;
}

static void MakeWindowResizable()
{
	LONG_PTR WindowStyle = GetWindowLongPtr(TwinkUiState::Window, GWL_STYLE);
	if ((WindowStyle & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW) return;
	SetWindowLongPtr(TwinkUiState::Window, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
}
#endif

static bool RunInputCallbacks(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using Gecko::CallbackType;

	Gecko::ScriptManager* Manager = Gecko::TheScriptManager;
	if (!Manager) return false;

	int X = (int)(short)LOWORD(lParam);
	int Y = (int)(short)HIWORD(lParam);

	auto Run = [&](CallbackType Void, CallbackType Blocking, const std::function<void(asIScriptContext*)>& SetArgs)
	{
		bool Blocked = Manager->RunAll(Void, SetArgs);
		return Manager->RunAll(Blocking, SetArgs) || Blocked;
	};

	auto Button = [&](bool Down, int Index)
	{
		return Run(CallbackType::OnMouseButton_VoidReturn, CallbackType::OnMouseButton_InputBlockingReturn, [&](asIScriptContext* Ctx)
		{
			Ctx->SetArgByte(0, Down);
			Ctx->SetArgDWord(1, (asDWORD)Index);
			Ctx->SetArgDWord(2, (asDWORD)X);
			Ctx->SetArgDWord(3, (asDWORD)Y);
		});
	};

	auto Wheel = [&](int& Remainder, bool Horizontal)
	{
		Remainder += GET_WHEEL_DELTA_WPARAM(wParam);
		int Notches = Remainder / WHEEL_DELTA;
		if (!Notches) return false;
		Remainder -= Notches * WHEEL_DELTA;

		return Run(CallbackType::OnMouseWheel_VoidReturn, CallbackType::OnMouseWheel_InputBlockingReturn, [&](asIScriptContext* Ctx)
		{
			Ctx->SetArgDWord(0, (asDWORD)(Horizontal ? Notches : 0));
			Ctx->SetArgDWord(1, (asDWORD)(Horizontal ? 0 : Notches));
		});
	};

	static int VerticalRemainder = 0;
	static int HorizontalRemainder = 0;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		bool Down = uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN;
		return Run(CallbackType::OnKeyPress_VoidReturn, CallbackType::OnKeyPress_InputBlockingReturn, [&](asIScriptContext* Ctx)
		{
			Ctx->SetArgByte(0, Down);
			Ctx->SetArgDWord(1, (asDWORD)wParam);
		});
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		return Button(true, 0);
	case WM_LBUTTONUP:
		return Button(false, 0);
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		return Button(true, 1);
	case WM_RBUTTONUP:
		return Button(false, 1);
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		return Button(true, 2);
	case WM_MBUTTONUP:
		return Button(false, 2);
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		return Button(true, GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 3 : 4);
	case WM_XBUTTONUP:
		return Button(false, GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 3 : 4);
	case WM_MOUSEMOVE:
		Manager->RunAll(CallbackType::OnMouseMove, [&](asIScriptContext* Ctx)
		{
			Ctx->SetArgDWord(0, (asDWORD)X);
			Ctx->SetArgDWord(1, (asDWORD)Y);
		});
		return false;
	case WM_MOUSEWHEEL:
		return Wheel(VerticalRemainder, false);
	case WM_MOUSEHWHEEL:
		return Wheel(HorizontalRemainder, true);
	default:
		return false;
	}
}

static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_ENDSESSION and wParam) Gecko::SaveSettingsOnExit("Windows is ending the session");

	if (!TwinkUiState::ImGuiInit)
	{
		return CallWindowProc(TwinkUiState::oWndProc, hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
#ifdef GAMEBOX
#ifndef TMU
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			TwinkUiState::WindowWidth = LOWORD(lParam);
			TwinkUiState::WindowHeight = HIWORD(lParam);
			ImGui::GetIO().DisplaySize = ImVec2((float)LOWORD(lParam), (float)HIWORD(lParam));
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	case WM_PAINT:
		if (TwinkUiState::ImGuiInit)
		{
			D3DPRESENT_PARAMETERS d3dpp = {};
			if (GetPresentParameters(TwinkUiState::Device, d3dpp) and d3dpp.Windowed)
			{
				MakeWindowResizable();

				if (TwinkUiState::WindowWidth and TwinkUiState::WindowHeight
					and (d3dpp.BackBufferWidth != (UINT)TwinkUiState::WindowWidth or d3dpp.BackBufferHeight != (UINT)TwinkUiState::WindowHeight))
				{
					d3dpp.BackBufferWidth = TwinkUiState::WindowWidth;
					d3dpp.BackBufferHeight = TwinkUiState::WindowHeight;

					TwinkUiState::Device->Reset(&d3dpp);

					VirtualWrite(O_V_PRESENT, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkPresent);
					VirtualWrite(O_V_RESET, (uintptr_t)TwinkUiState::Device, (uintptr_t)hkReset);
				}
			}
		}
		break;
#endif
#endif
	case WM_FILE_SELECTED:
		break;
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	ImGuiIO& io = ImGui::GetIO();
	if (TwinkUiState::RenderUi)
	{
		if (io.WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST))
			return TRUE;

		if (io.WantCaptureMouse && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST))
			return TRUE;
	}

	if (RunInputCallbacks(uMsg, wParam, lParam))
		return TRUE;

	return CallWindowProc(TwinkUiState::oWndProc, hWnd, uMsg, wParam, lParam);
}

#ifdef GAMEBOX
void InitImGui(DirectXDevice* Device)
{
	static std::string ImGuiIniFilePath = (TwinkUiState::UiMgr->DocumentsFolderPath / L"TwinkieX\\imgui.ini").string();

	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& ImGuiIo = ImGui::GetIO();

	ImGuiIo.IniFilename = ImGuiIniFilePath.c_str();

	// The default font, with the icons merged into it
	ImGuiIo.Fonts->AddFontDefault();
	TwinkUiState::UiMgr->MergeIcons();

	ImGui_ImplWin32_Init(TwinkUiState::Window);
	ImGui_ImplDX9_Init(Device);
}

static long __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, LPVOID A, LPVOID B, HWND C, LPVOID D)
{
	static bool MustUpdateFont = false;

	if (!TwinkUiState::ImGuiInit)
	{
		// This runs on the game thread, which owns both the device and the window
		D3DDEVICE_CREATION_PARAMETERS D3DCreationParams = {};
		if (pDevice->GetCreationParameters(&D3DCreationParams) != D3D_OK or !D3DCreationParams.hFocusWindow)
		{
			// Try again next frame
			return TwinkUiState::oPresent(pDevice, A, B, C, D);
		}

		// Get the window from the creation parameters
		TwinkUiState::Window = D3DCreationParams.hFocusWindow;

		// Set our new window process
		TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);
		// TMU already resizes the window by itself
#ifndef TMU
		D3DPRESENT_PARAMETERS PresentParams = {};
		if (GetPresentParameters(pDevice, PresentParams) and PresentParams.Windowed) MakeWindowResizable();
#endif

		LoggertTwink.verbose(LogFormat("First Present on thread {}, window {:#x}", GetCurrentThreadId(), (uintptr_t)TwinkUiState::Window));
		InitImGui(pDevice);
		TwinkNvg::Init();
		TwinkUiState::ImGuiInit = true;
		MustUpdateFont = true;
		LoggertTwink.print(u8"Overlay initialized");
	}

	if (pDevice->TestCooperativeLevel() != D3D_OK) return TwinkUiState::oPresent(pDevice, A, B, C, D);
	TwinkNvg::Init();

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

		IDirect3DSurface9* Target = nullptr;
		if (SUCCEEDED(pDevice->GetRenderTarget(0, &Target)) and Target)
		{
			D3DSURFACE_DESC TargetDesc;
			if (SUCCEEDED(Target->GetDesc(&TargetDesc))) TwinkNvg::BeginFrame((float)TargetDesc.Width, (float)TargetDesc.Height);
			Target->Release();
		}

		if (MustUpdateFont and not TwinkUiState::UiMgr->FontPath.empty())
		{
			TwinkUiState::UiMgr->Font = TwinkUiState::UiMgr->AddFont((char*)TwinkUiState::UiMgr->FontPath.c_str());
		}
		MustUpdateFont = false;

		TwinkUiState::UiMgr->Render();

		TwinkNvg::EndFrame();

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		Texture::ReleasePending();

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

#ifdef O_V_RESET
static long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pParams)
{
	if (!TwinkUiState::ImGuiInit) return TwinkUiState::oReset(pDevice, pParams);
#ifndef TMU
	if (pParams and pParams->Windowed and TwinkUiState::WindowWidth and TwinkUiState::WindowHeight)
	{
		pParams->BackBufferWidth = TwinkUiState::WindowWidth;
		pParams->BackBufferHeight = TwinkUiState::WindowHeight;
	}
#endif
	ImGui_ImplDX9_InvalidateDeviceObjects();
	TwinkNvg::Invalidate();
	const HRESULT result = TwinkUiState::oReset(pDevice, pParams);
	if (SUCCEEDED(result))
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		TwinkNvg::Init();
	}
	return result;
}
#endif
#elif defined(MANIAPLANET)
void InitImGui(DirectXContext* Context, DirectXDevice* Device)
{
	ImGui::CreateContext();
	ImPlot::CreateContext();

	// The default font, with the icons merged into it
	ImGui::GetIO().Fonts->AddFontDefault();
	TwinkUiState::UiMgr->MergeIcons();

	ImGui_ImplWin32_Init(TwinkUiState::Window);
	ImGui_ImplDX11_Init(Device, Context);
}

// Creates the render target view for the swap chain's current back buffer
static bool CreateMainRenderTargetView(IDXGISwapChain* pSwapChain)
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)) or not pBackBuffer) return false;

	// Get the color format from the swap chain's back buffer for use with the render target view
	D3D11_TEXTURE2D_DESC BackBufferDesc;
	pBackBuffer->GetDesc(&BackBufferDesc);
	DXGI_FORMAT BackBufferColorFmt = BackBufferDesc.Format;

	// Remove sRGB from the backbuffer color format, since imgui already uses sRGB
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

	HRESULT Result = TwinkUiState::Device->CreateRenderTargetView(pBackBuffer, &RenderTargetViewDesc, &TwinkUiState::MainRenderTargetView);

	// Release the buffer in every case: a reference left on it makes every later ResizeBuffers fail
	pBackBuffer->Release();

	return SUCCEEDED(Result);
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

			// Create a target view using the back buffer to render our UI on
			if (not CreateMainRenderTargetView(pSwapChain)) return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);

			// Hook the window process
			TwinkUiState::oWndProc = (WNDPROC)SetWindowLongPtr(TwinkUiState::Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			LoggertTwink.verbose(LogFormat("First Present on thread {}, window {:#x}, device {:#x}", GetCurrentThreadId(), (uintptr_t)TwinkUiState::Window, (uintptr_t)TwinkUiState::Device));

			// Initialize ImGui
			InitImGui(TwinkUiState::Context, TwinkUiState::Device);
			TwinkNvg::Init();
			LoggertTwink.print(u8"Overlay initialized");
		}

		// If we couldn't get the device, then fallback to original behavior
		else
		{
			static bool Warned = false;
			if (!Warned)
			{
				Warned = true;
				LoggertTwink.warn(u8"Couldn't get the D3D11 device from the swap chain, retrying every frame");
			}
			return TwinkUiState::oPresent(pSwapChain, SyncInterval, Flags);
		}
	}

	// Create a new frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	if (SUCCEEDED(pSwapChain->GetDesc(&SwapChainDesc))) TwinkNvg::BeginFrame((float)SwapChainDesc.BufferDesc.Width, (float)SwapChainDesc.BufferDesc.Height);

	if (not TwinkUiState::ImGuiInit)
	{
		if (not TwinkUiState::UiMgr->FontPath.empty())
		{
			TwinkUiState::UiMgr->Font = TwinkUiState::UiMgr->AddFont((char*)TwinkUiState::UiMgr->FontPath.c_str());
		}
		TwinkUiState::ImGuiInit = true;
	}

	TwinkUiState::UiMgr->Render();

	if (not TwinkUiState::MainRenderTargetView) CreateMainRenderTargetView(pSwapChain);
	TwinkNvg::EndFrame();

	ImGui::Render();

	// The view is released when the buffers are resized, so make one for the new back buffer
	if (TwinkUiState::MainRenderTargetView or CreateMainRenderTargetView(pSwapChain))
	{
		// Set the render target for the draw data rendering
		TwinkUiState::Context->OMSetRenderTargets(1, &TwinkUiState::MainRenderTargetView, NULL);

		// Draw from the draw data
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		Texture::ReleasePending();
	}

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

HRESULT hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	// ResizeBuffers fails while anything still references the back buffer, which leaves the game with its old,
	// smaller buffer. So let go of our view of it first.
	if (TwinkUiState::MainRenderTargetView)
	{
		TwinkUiState::Context->OMSetRenderTargets(0, nullptr, nullptr);
		TwinkUiState::MainRenderTargetView->Release();
		TwinkUiState::MainRenderTargetView = nullptr;

		// D3D11 destroys released objects lazily, so without a flush the view can still hold the buffer
		TwinkUiState::Context->Flush();
	}

	// The view for the new back buffer is made on the next Present, so the game's state isn't touched here
	return TwinkUiState::oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}
#endif

// END Methods