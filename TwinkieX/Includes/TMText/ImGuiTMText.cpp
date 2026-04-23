#include "pch.h"

#pragma warning(disable : 4062 4244 4242)
#include "ImGuiTMText.h"

namespace ImGui
{
	bool FormatState::ApplyFormatSpecifier(const TrackmaniaText::FormatSpecifier& Specifier)
	{
		using namespace TrackmaniaText;

		std::string CodeAsValidHex = "";

		switch (Specifier.Type)
		{
		case FormatSpecifierType_Bold:
			this->Bold = !this->Bold;
			break;
		case FormatSpecifierType_Italic:
			this->Italic = !this->Italic;
			break;
		case FormatSpecifierType_ForceUpper:
			this->ForceUpper = !this->ForceUpper;
			break;
		case FormatSpecifierType_Shadow:
			this->Shadow = !this->Shadow;
			break;
		case FormatSpecifierType_Narrow:
			this->Narrow = !this->Narrow;
			break;
		case FormatSpecifierType_Wide:
			this->Wide = !this->Wide;
			break;
		case FormatSpecifierType_Color:
			this->Color = 0;

			for (size_t CharIdx = Specifier.Code.length(); CharIdx != 0; CharIdx--)
			{
				char Char = Specifier.Code[CharIdx - 1];

				if (Char == '$') continue;
				if (not (TrackmaniaText_IsHexadecimal(Char)))
				{
					CodeAsValidHex += '0';
				}
				else CodeAsValidHex = CodeAsValidHex + Char + Char;
			}

			this->Color = std::stoul(CodeAsValidHex, nullptr, 16);
			this->Color = this->Color | 0xFF000000;
		default:
			return false;
		}

		return true;
	}

	void FormatChunk::Render()
	{
		// TODO: Rest of format specifiers

		std::string RenderedString = String;
		if (State.ForceUpper) std::transform(String.begin(), String.end(), String.begin(), ::toupper);

		PushStyleColor(ImGuiCol_Text, ColorConvertU32ToFloat4(State.Color));

		Text(String.c_str());

		PopStyleColor();
	}

    void TextTrackmania(const char* Text)
    {
        using namespace TrackmaniaText;
        ImGuiStyle& Style = GetStyle();

        FormatSpecifierSearchResult SearchResult = FindAllFormatSpecifiers(Text);

        FormatState CurrentState;
        CurrentState.Color = ColorConvertFloat4ToU32(Style.Colors[ImGuiCol_Text]);
        std::vector<FormatState> StateStack;

        size_t Cursor = 0;
        bool IsFirstChunk = true;

        for (const auto& Specifier : SearchResult.Specifiers)
        {
            if (Specifier.Idx > Cursor)
            {
                std::string Part = SearchResult.OriginalText.substr(Cursor, Specifier.Idx - Cursor);

                if (!IsFirstChunk) SameLine(0, 0);
                FormatChunk{ CurrentState, Part }.Render();
                IsFirstChunk = false;
            }

            if (!CurrentState.ApplyFormatSpecifier(Specifier))
            {
                switch (Specifier.Type)
                {
                case FormatSpecifierType_ResetColor:
                    CurrentState.Color = ColorConvertFloat4ToU32(Style.Colors[ImGuiCol_Text]);
                    break;
                case FormatSpecifierType_ResetAll:
                    CurrentState = {};
                    CurrentState.Color = ColorConvertFloat4ToU32(Style.Colors[ImGuiCol_Text]);
                    break;
                case FormatSpecifierType_BracketBegin:
                    StateStack.push_back(CurrentState);
                    break;
                case FormatSpecifierType_BracketEnd:
                    if (!StateStack.empty()) {
                        CurrentState = StateStack.back();
                        StateStack.pop_back();
                    }
                    break;
                }
            }

            Cursor = Specifier.Idx;
        }

        if (Cursor < SearchResult.OriginalText.length())
        {
            std::string Remaining = SearchResult.OriginalText.substr(Cursor);

            if (!IsFirstChunk) SameLine(0, 0);
            FormatChunk{ CurrentState, Remaining }.Render();
        }
    }
}