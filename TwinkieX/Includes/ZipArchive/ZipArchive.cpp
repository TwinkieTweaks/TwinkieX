#include "pch.h"
#include "ZipArchive.h"

ZipContainedFile::ZipContainedFile(zip_file_t* File, zip_stat_t& Entry)
{
	this->ZipFile = File;
	this->Entry = Entry;
	this->Filename = (const char8_t*)Entry.name;
	this->Size = Entry.size;
	this->Data.reserve(Size);
	this->Data.resize(Size);

	zip_fread(File, this->Data.data(), Size);
}

void ZipContainedFile::Close()
{
	zip_fclose(*this);
}

ZipArchive::ZipArchive(const std::filesystem::path& Path)
{
	int Err = 0;
	LibZipZipFile = zip_open(Path.string().c_str(), ZIP_RDONLY, &Err);
	if (!LibZipZipFile) return;	
	
	uint64_t EntriesCount = zip_get_num_entries(LibZipZipFile, 0);
	Entries.reserve(EntriesCount);
	Entries.resize(EntriesCount);
	for (uint64_t EntryIdx = 0; EntryIdx < EntriesCount; EntryIdx++)
	{
		zip_stat_t Entry;
		if (zip_stat_index(LibZipZipFile, EntryIdx, 0, &Entry) != 0) continue;
		
		Entries[EntryIdx] = Entry;

		if (Entry.name[strlen(Entry.name) - 1] == '/') continue;

		zip_file_t* File = zip_fopen_index(LibZipZipFile, EntryIdx, 0);
		if (!File) continue;

		Files.push_back({ File, Entry });
	}
}

ZipArchive::~ZipArchive()
{
	for (auto& File : *this)
	{
		File.Close();
	}

	zip_close(LibZipZipFile);
	LibZipZipFile = nullptr;
}

std::vector<ZipContainedFile>::iterator ZipArchive::begin()
{
	return Files.begin();
}

std::vector<ZipContainedFile>::iterator ZipArchive::end()
{
	return Files.end();
}