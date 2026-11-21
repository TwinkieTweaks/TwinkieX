#pragma once
#include "pch.h"

namespace Gecko::Exports::ImGui
{
	void Registrar(asIScriptEngine*);
}

// UI::Font
class ScriptFont
{
	std::string Key;
	std::vector<uint8_t> Data;
	std::vector<ImWchar> GlyphRanges;
	mutable size_t RefCount = 0;

	friend void Gecko::Exports::ImGui::Registrar(asIScriptEngine*);

	void AddRef();
	void RemoveRef();

	ScriptFont(std::string Key, std::vector<uint8_t> Data, float Size, std::vector<ImWchar> GlyphRanges);

	static ScriptFont* LoadDefault(const wchar_t* Filename);

public:
	float Size = 16.0f;

	static ScriptFont* Load(const std::string& Filename, float Size, std::vector<ImWchar> GlyphRanges = {});
	static ScriptFont* LoadSystem(const std::string& Filename);

	static ScriptFont* GetDefault();
	static ScriptFont* GetDefaultBold();
	static ScriptFont* GetDefaultMono();

	ImFont* Get();
};
