#pragma once
#include "pch.h"

class Texture
{
public:
	Texture() = delete;
	Texture(void* Memory, size_t Size);
	Texture(const std::filesystem::path& Path);
};