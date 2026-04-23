#include "pch.h"
#include "TMText.h"

namespace TrackmaniaText
{
	std::string ValidTrackmaniaTextFormatSpecifierChars = "<>ihownmtslgzpabcdef0123456789";

	constexpr char CharToLower(char Char)
	{
		return Char >= 'A' and Char <= 'Z' ? Char | 32 : Char;
	}

	FormatSpecifier::FormatSpecifier(std::string FormatSpec, size_t Idx)
	{
		char TypeOfSpecifierDecider = CharToLower(FormatSpec[1]);
		if (TrackmaniaText_IsHexadecimal(TypeOfSpecifierDecider))
		{
			this->Type = FormatSpecifierType_Color;
		}
		else if (ValidTrackmaniaTextFormatSpecifierChars.find(TypeOfSpecifierDecider) == std::string::npos)
		{
			this->Type = FormatSpecifierType_NotSupported;
		}
		else
		{
			this->Type = (FormatSpecifierType)TypeOfSpecifierDecider;
		}

		this->Code = FormatSpec;
		this->Idx = Idx;
#ifdef _M_X64
		this->Pad = 0;
#endif
	}

	void FormatSpecifierSearchResult::push_back(std::string FormatSpec, size_t Idx)
	{
		Specifiers.push_back(FormatSpecifier( FormatSpec, Idx ));
	}

	std::vector<FormatSpecifier>::iterator FormatSpecifierSearchResult::begin()
	{
		return Specifiers.begin();
	}

	std::vector<FormatSpecifier>::iterator FormatSpecifierSearchResult::end()
	{
		return Specifiers.end();
	}

    FormatSpecifierSearchResult FindAllFormatSpecifiers(std::string Str)
    {
        FormatSpecifierSearchResult Founds = {};

        int CharsToGo = 0;
        std::string Temp = "";

        for (size_t CharIdx = 0; CharIdx < Str.length(); CharIdx++)
        {
            char Char = Str[CharIdx];
            bool CharInvolvedInSpecifiers = false;

            if (CharsToGo == 0)
            {
                if (!Temp.empty())
                {
                    Founds.push_back(Temp, Founds.OriginalText.length());
                    Temp = "";
                }
                CharsToGo = 1;
            }

            if (Char == '$' && Temp.empty())
            {
                CharsToGo = 2;
                Temp.push_back(Char);
                CharInvolvedInSpecifiers = true;
            }
            else if (Char == '$' && Temp == "$")
            {
                CharsToGo = 1;
                Temp = "";
            }
            else if (Temp.length() == 1)
            {
                if (TrackmaniaText_IsHexadecimal(Char))
                {
                    CharsToGo = 3;
                }
                Temp.push_back(Char);
                CharInvolvedInSpecifiers = true;
            }
            else if (Temp.length() > 1)
            {
                Temp.push_back(Char);
                CharInvolvedInSpecifiers = true;
            }

            CharsToGo -= 1;

            if (!CharInvolvedInSpecifiers)
            {
                Founds.OriginalText.push_back(Char);
            }
        }

        if (Temp.length() > 1)
        {
            Founds.push_back(Temp, Founds.OriginalText.length());
        }

        return Founds;
    }
}