#pragma once
#include "pch.h"

#ifdef X64
using TextureResource = ID3D11ShaderResourceView*;
#else
using TextureResource = IDirect3DTexture9*;
#endif

class Texture
{
	TextureResource InternalTexture = nullptr;
	size_t Width, Height = 0;
public:
	Texture();
	Texture(void* Memory, size_t Size);
	Texture(const std::filesystem::path& Path);
	~Texture();

	bool From(void* Memory, size_t Size);
	bool From(const std::filesystem::path& Path);
	ImTextureID Get() const;
};