#include "pch.h"
#include "ResourceLoader.h"

ResourceLoader::ResourceLoader(int Id)
{
	Load(Id);
}

ResourceLoader::ResourceLoader() {}

void ResourceLoader::Load(int Id)
{
	auto TwinkieXModule = GetModuleHandleW(TWINKIEX_DLLW);
	auto Resource = FindResource(TwinkieXModule, MAKEINTRESOURCE(Id), RT_RCDATA);
	if (!Resource) return;

	auto ResourceHandle = LoadResource(TwinkieXModule, Resource);
	if (!ResourceHandle or ResourceHandle == INVALID_HANDLE_VALUE) return;

	auto ResourceData = (uint8_t*)LockResource(ResourceHandle);
	auto ResourceSize = SizeofResource(TwinkieXModule, Resource);

	Data.assign(ResourceData, ResourceData + ResourceSize);

	FreeResource(ResourceHandle);
}