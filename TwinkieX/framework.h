#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN  

// This warning always fires when including Windows.h for some reason
#pragma warning(disable : 5039)

// "relative include path contains '..'"
#pragma warning(disable : 4464)

// Windows Header Files
#include <windows.h>

// Architecture detection header file
#include <Utils/Arch.h>

// Disable all warnings emmitted by DirectX11
#pragma warning(disable : 4061 4365 4820 4365 4865)

// DirectX (11 for TMCN, 9 otherwise)
#ifdef MANIAPLANET

#include <d3d11.h>
#include <dxgi.h>

#else

#include <d3d9.h>

#endif

// dear ImGui
// No idea why this define is needed
#define IMGUI_DEFINE_MATH_OPERATORS

// Disable all warnings emmitted by dear ImGui
#pragma warning(disable : 4820 26819 28182 4191 4365 4582 4774 5219 5045 4738 4710 5262 4711)

#include <imgui/imgui.h>

#ifdef GAMEBOX
#include <imgui/imgui_impl_dx9.h>
#else
#include <imgui/imgui_impl_dx11.h>
#endif

#include <imgui/imgui_impl_win32.h>

#include <GlyphTable/IconsForkAwesome.h>
#include <GlyphTable/IconsKenney.h>

#include <vector>