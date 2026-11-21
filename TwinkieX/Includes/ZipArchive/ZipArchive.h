#pragma once
#include "pch.h"

class ZipArchive;

class ZipContainedFile
{
	friend class ZipArchive;

	zip_file_t* ZipFile = nullptr;
	zip_stat_t Entry{};
public:
	const char8_t* Filename = u8"";
	uint64_t Size = 0;
	std::vector<char8_t> Data;

	ZipContainedFile(zip_file_t*, zip_stat_t&);

	operator zip_file_t* () { return ZipFile; }
	void Close();
};

class ZipArchive
{
	zip_t* LibZipZipFile = nullptr;
	std::vector<zip_stat_t> Entries;
	std::vector<ZipContainedFile> Files;
public:
	ZipArchive() = delete;
	ZipArchive(const std::filesystem::path&);
	~ZipArchive();

	std::vector<ZipContainedFile>::iterator begin();
	std::vector<ZipContainedFile>::iterator end();
};