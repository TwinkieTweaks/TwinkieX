#pragma once
#include "resource.h"
#include "pch.h"

class ResourceLoader
{
public:
	std::vector<uint8_t> Data;

	ResourceLoader(int Id);
	ResourceLoader();

	void Load(int Id);
};