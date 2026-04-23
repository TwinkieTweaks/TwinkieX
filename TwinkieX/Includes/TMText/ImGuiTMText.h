#include "TMText.h"
#include <imgui/imgui.h>
#pragma once

namespace ImGui
{
	struct FormatState
	{
		bool Bold = false;
		bool Italic = false;
		bool ForceUpper = false;
		bool Shadow = false;
		bool Narrow = false;
		bool Wide = false;
		uint32_t Color = 0;

		bool ApplyFormatSpecifier(const TrackmaniaText::FormatSpecifier& Specifier);
	};

	struct FormatChunk
	{
		FormatState State = {};
		std::string String = "";

		void Render();
	};

	void TextTrackmania(const char* Text);
}