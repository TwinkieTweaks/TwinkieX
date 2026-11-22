#include "pch.h"
#include "ExportIO.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/Exports/Global/MemoryBuffer/MemoryBuffer.h>
#include <Gecko/ScriptFiles.h>

namespace Gecko::Exports::IO
{
	static std::vector<GeckoExport> s_Exports;
#define FILESOURCE_BEHAVIOUR(Beh_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "IO", "FileSource", Beh_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define FILESOURCE_METHOD(Decl_, Fn_) GECKO_EXPORT_METHOD(s_Exports, "IO", "FileSource", Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)

	struct FileSource
	{
		std::vector<uint8_t> Data;
		size_t Pos = 0;

		size_t Remaining() const { return Data.size() - Pos; }
	};

	static void Construct(const std::string& Filename, FileSource* Self)
	{
		new (Self) FileSource();
		if (not Gecko::ReadScriptFile(std::filesystem::path((const char8_t*)Filename.c_str()), Self->Data))
		{
			if (auto Ctx = asGetActiveContext()) Ctx->SetException(("Could not open file source: " + Filename).c_str());
		}
	}

	static void ConstructCopy(const FileSource& Other, FileSource* Self) { new (Self) FileSource(Other); }
	static void Destruct(FileSource* Self) { Self->~FileSource(); }
	static FileSource& Assign(const FileSource& Other, FileSource* Self) { return *Self = Other; }

	static uint64_t Size(FileSource* Self) { return Self->Data.size(); }
	static bool Eof(FileSource* Self) { return Self->Pos >= Self->Data.size(); }
	static void SetPos(uint64_t Pos, FileSource* Self) { Self->Pos = (size_t)(std::min)(Pos, (uint64_t)Self->Data.size()); }
	static uint64_t GetPos(FileSource* Self) { return Self->Pos; }

	static MemoryBuffer* Read(uint64_t Size, FileSource* Self)
	{
		size_t Count = (size_t)(std::min)(Size, (uint64_t)Self->Remaining());
		MemoryBuffer* Buffer = new MemoryBuffer(Self->Data.data() + Self->Pos, Count);
		Self->Pos += Count;
		return Buffer;
	}

	static std::string ReadLine(FileSource* Self)
	{
		const char* Begin = (const char*)Self->Data.data() + Self->Pos;
		const char* End = (const char*)Self->Data.data() + Self->Data.size();
		const char* Newline = std::find(Begin, End, '\n');

		std::string Line(Begin, Newline);
		if (not Line.empty() and Line.back() == '\r') Line.pop_back();

		Self->Pos = Newline == End ? Self->Data.size() : (size_t)(Newline - (const char*)Self->Data.data()) + 1;
		return Line;
	}

	static std::string ReadToEnd(FileSource* Self)
	{
		std::string Rest((const char*)Self->Data.data() + Self->Pos, Self->Remaining());
		Self->Pos = Self->Data.size();
		return Rest;
	}

	/*
	!CompatMeta
	for     IO
	with    FileSource(const string&in filename)
	with    FileSource(const IO::FileSource&in)
	with    ~FileSource()
	being   behavior
	of      FileSource
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	FILESOURCE_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(const string&in filename)", Construct);
	FILESOURCE_BEHAVIOUR(asBEHAVE_CONSTRUCT, "void f(const IO::FileSource&in)", ConstructCopy);
	FILESOURCE_BEHAVIOUR(asBEHAVE_DESTRUCT, "void f()", Destruct);

	/*
	!CompatMeta
	for     IO
	with    uint64 Size()
	with    bool EOF()
	with    void SetPos(uint64 pos)
	with    uint64 Pos()
	with    MemoryBuffer@ Read(uint64 size)
	with    string ReadLine()
	with    string ReadToEnd()
	being   method
	of      FileSource
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	FILESOURCE_METHOD("IO::FileSource& opAssign(const IO::FileSource&in)", Assign);
	FILESOURCE_METHOD("uint64 Size()", Size);
	FILESOURCE_METHOD("bool EOF()", Eof);
	FILESOURCE_METHOD("void SetPos(uint64 pos)", SetPos);
	FILESOURCE_METHOD("uint64 Pos()", GetPos);
	FILESOURCE_METHOD("MemoryBuffer@+ Read(uint64 size)", Read);
	FILESOURCE_METHOD("string ReadLine()", ReadLine);
	FILESOURCE_METHOD("string ReadToEnd()", ReadToEnd);

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("IO");
		Engine->RegisterObjectType("FileSource", sizeof(FileSource), asOBJ_VALUE | asGetTypeTraits<FileSource>());
		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_Exports);
	}
}
