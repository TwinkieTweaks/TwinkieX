#pragma once
#pragma warning(disable : 4242 5045 4711 4710)
#pragma warning(push)

#include <string>
#include <regex>
#include <vector>
#include <algorithm>

#pragma warning(pop)

#define TrackmaniaText_IsHexadecimal(Char) ((Char) >= '0' and (Char) <= '9') or ((Char) >= 'a' and (Char) <= 'f') or ((Char) >= 'A' and (Char) <= 'F')


namespace TrackmaniaText
{
	extern std::string ValidTrackmaniaTextFormatSpecifierChars;
	
	constexpr char CharToLower(char);

	enum FormatSpecifierType
	{
		FormatSpecifierType_NotSupported = 0,
		FormatSpecifierType_Color = 1,
		FormatSpecifierType_Italic = 'i',
		FormatSpecifierType_Bold = 'o',
		FormatSpecifierType_Wide = 'w',
		FormatSpecifierType_Narrow = 'n',
		FormatSpecifierType_ResetWidth = 'm',
		FormatSpecifierType_ForceUpper = 't',
		FormatSpecifierType_Shadow = 's',
		FormatSpecifierType_Link = 'l',
		FormatSpecifierType_ResetColor = 'g',
		FormatSpecifierType_ResetAll = 'z',
		FormatSpecifierType_PlayerManialink = 'p',
		FormatSpecifierType_InternalLink = 'h',
		FormatSpecifierType_BracketBegin = '<',
		FormatSpecifierType_BracketEnd = '>'
	};

	struct FormatSpecifier
	{
		std::string Code;
		FormatSpecifierType Type;
#ifdef _M_X64
		uint32_t Pad;
#endif
		size_t Idx;

		FormatSpecifier(std::string FormatSpec, size_t Idx = -1);
	};

	struct TextSlice
	{
		std::string Text;
		FormatSpecifier Specifier;

		TextSlice() = delete;
	};

	struct FormatSpecifierSearchResult
	{
		std::vector<FormatSpecifier> Specifiers;
		std::string OriginalText;
		std::string TextNoFormatSpecifiers;

		void push_back(std::string FormatSpec, size_t Idx = -1);

		std::vector<FormatSpecifier>::iterator begin();
		std::vector<FormatSpecifier>::iterator end();
	};

	FormatSpecifierSearchResult FindAllFormatSpecifiers(std::string Str);
}

