
#include "pch.h"
#include "Global.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include "MemoryBuffer/MemoryBuffer.h"

namespace Gecko::Exports::Global
{
	static std::vector<GeckoExport> s_Exports;
#define GLOBAL_EXPORT(Decl_, Fn_) GECKO_EXPORT_FN(s_Exports, "", Decl_, Fn_)
#define GLOBAL_MEMORYBUFFER_METHOD(Decl_, Ptr_, Conv_)          GECKO_EXPORT_METHOD(s_Exports, "", "MemoryBuffer", Decl_, Ptr_, Conv_)
#define GLOBAL_MEMORYBUFFER_BEHAVIOUR(Beh_, Decl_, Ptr_, Conv_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "", "MemoryBuffer", Beh_, Decl_, Ptr_, Conv_)

	/*
	!CompatMeta
	for     Global
	with    void yield()
	as      void yield()
	being   function
	declare full
	since   v0.1.0
	!CompatMeta
	*/
	void yield()
	{
		if (WaitState* Wait = SuspendCurrentContext()) Wait->Frames = 0;
	}
	GLOBAL_EXPORT("void yield()", yield);

	/*
	!CompatMeta
	for     Global
	with    void yield(uint frames)
	as      void yield(uint frames)
	being   function
	declare full
	since   v0.1.0
	!CompatMeta
	*/
	void yieldFor(uint32_t For)
	{
		if (!For) return;
		if (WaitState* Wait = SuspendCurrentContext()) Wait->Frames = For - 1;
	}
	GLOBAL_EXPORT("void yield(uint)", yieldFor);

	/*
	!CompatMeta
	for     Global
	with    void trace(const string&in)
	as      void trace(const string&in)
	being   function
	declare full
	!CompatMeta
	*/
	void trace(std::u8string& Str)
	{
		LoggertAngelscript.trace((char8_t*)std::format("[{}] {}", (char*)Gecko::TheScriptManager->GetActiveScript()->Id.c_str(), (char*)Str.c_str()).c_str());
	}
	GLOBAL_EXPORT("void trace(const string &in)", trace);

	/*
	!CompatMeta
	for     Global
	with    void print(const string&in)
	as      void print(const string&in)
	being   function
	declare full
	!CompatMeta
	*/
	void print(std::u8string& Str)
	{
		LoggertAngelscript.print((char8_t*)std::format("[{}] {}", (char*)Gecko::TheScriptManager->GetActiveScript()->Id.c_str(), (char*)Str.c_str()).c_str());
	}
	GLOBAL_EXPORT("void print(const string &in)", print);

	/*
	!CompatMeta
	for     Global
	with    void warn(const string&in)
	as      void warn(const string&in)
	being   function
	declare full
	!CompatMeta
	*/
	void warn(std::u8string& Str)
	{
		LoggertAngelscript.warn((char8_t*)std::format("[{}] {}", (char*)Gecko::TheScriptManager->GetActiveScript()->Id.c_str(), (char*)Str.c_str()).c_str());
	}
	GLOBAL_EXPORT("void warn(const string &in)", warn);

	/*
	!CompatMeta
	for     Global
	with    void error(const string&in)
	as      void error(const string&in)
	being   function
	declare full
	!CompatMeta
	*/
	void error(std::u8string& Str)
	{
		LoggertAngelscript.error((char8_t*)std::format("[{}] {}", (char*)Gecko::TheScriptManager->GetActiveScript()->Id.c_str(), (char*)Str.c_str()).c_str());
	}
	GLOBAL_EXPORT("void error(const string &in)", error);

	static bool CallToString(asIScriptEngine* Engine, asIScriptFunction* Method, void* Object, std::string& Out)
	{
		asIScriptContext* Ctx = asGetActiveContext();
		bool Nested = Ctx and Ctx->GetEngine() == Engine and Ctx->PushState() >= 0;
		if (not Nested) Ctx = Engine->RequestContext();
		if (not Ctx) return false;

		bool Done = Ctx->Prepare(Method) >= 0 and Ctx->SetObject(Object) >= 0 and Ctx->Execute() == asEXECUTION_FINISHED;
		if (Done) Out = *(std::string*)Ctx->GetReturnObject();

		if (Nested) Ctx->PopState();
		else Engine->ReturnContext(Ctx);
		return Done;
	}

	static std::string TypeName(asITypeInfo* Type)
	{
		std::string Namespace = Type->GetNamespace() ? Type->GetNamespace() : "";
		return Namespace.empty() ? Type->GetName() : Namespace + "::" + Type->GetName();
	}

	static std::string ValueToString(asIScriptEngine* Engine, void* Ref, int TypeId)
	{
		switch (TypeId)
		{
		case asTYPEID_BOOL:   return *(const bool*)Ref ? "true" : "false";
		case asTYPEID_INT8:   return std::to_string(*(const int8_t*)Ref);
		case asTYPEID_INT16:  return std::to_string(*(const int16_t*)Ref);
		case asTYPEID_INT32:  return std::to_string(*(const int32_t*)Ref);
		case asTYPEID_INT64:  return std::to_string(*(const int64_t*)Ref);
		case asTYPEID_UINT8:  return std::to_string(*(const uint8_t*)Ref);
		case asTYPEID_UINT16: return std::to_string(*(const uint16_t*)Ref);
		case asTYPEID_UINT32: return std::to_string(*(const uint32_t*)Ref);
		case asTYPEID_UINT64: return std::to_string(*(const uint64_t*)Ref);
		case asTYPEID_FLOAT:  return std::format("{}", *(const float*)Ref);
		case asTYPEID_DOUBLE: return std::format("{}", *(const double*)Ref);
		default: break;
		}

		asITypeInfo* Type = Engine->GetTypeInfoById(TypeId);
		if (not Type) return {};

		if (Type->GetFlags() & asOBJ_ENUM)
		{
			int Value = *(const int*)Ref;
			for (asUINT i = 0; i < Type->GetEnumValueCount(); i++)
			{
				int EnumValue = 0;
				const char* Name = Type->GetEnumValueByIndex(i, &EnumValue);
				if (EnumValue == Value) return Name;
			}
			return std::to_string(Value);
		}

		void* Object = (TypeId & asTYPEID_OBJHANDLE) ? *(void**)Ref : Ref;
		if (not Object) return "null";

		if (std::strcmp(Type->GetName(), "string") == 0 and not (TypeId & asTYPEID_OBJHANDLE)) return *(const std::string*)Object;

		asIScriptFunction* Method = Type->GetMethodByDecl("string ToString() const");
		if (not Method) Method = Type->GetMethodByDecl("string ToString()");

		std::string Out;
		if (Method and CallToString(Engine, Method, Object, Out)) return Out;

		return TypeName(Type);
	}

	/*
	!CompatMeta
	for     Global
	with    string tostring(const ?&in)
	as      string tostring(const ?&in)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void tostring(asIScriptGeneric* Gen)
	{
		std::string Result = ValueToString(Gen->GetEngine(), Gen->GetArgAddress(0), Gen->GetArgTypeId(0));
		Gen->SetReturnObject(&Result);
	}
	GECKO_EXPORT_FN_EX(s_Exports, "", "string tostring(const ?&in)", asFUNCTION(tostring), asCALL_GENERIC);

	/*
	!CompatMeta
	for     Global
	with    void trace(?&in)
	as      void trace(?&in)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void traceAny(asIScriptGeneric* Gen)
	{
		std::string Message = ValueToString(Gen->GetEngine(), Gen->GetArgAddress(0), Gen->GetArgTypeId(0));
		std::u8string Str(Message.begin(), Message.end());
		trace(Str);
	}
	GECKO_EXPORT_FN_EX(s_Exports, "", "void trace(?&in)", asFUNCTION(traceAny), asCALL_GENERIC);

	/*
	!CompatMeta
	for     Global
	with    void print(?&in)
	as      void print(?&in)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void printAny(asIScriptGeneric* Gen)
	{
		std::string Message = ValueToString(Gen->GetEngine(), Gen->GetArgAddress(0), Gen->GetArgTypeId(0));
		std::u8string Str(Message.begin(), Message.end());
		print(Str);
	}
	GECKO_EXPORT_FN_EX(s_Exports, "", "void print(?&in)", asFUNCTION(printAny), asCALL_GENERIC);

	/*
	!CompatMeta
	for     Global
	with    void warn(?&in)
	as      void warn(?&in)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void warnAny(asIScriptGeneric* Gen)
	{
		std::string Message = ValueToString(Gen->GetEngine(), Gen->GetArgAddress(0), Gen->GetArgTypeId(0));
		std::u8string Str(Message.begin(), Message.end());
		warn(Str);
	}
	GECKO_EXPORT_FN_EX(s_Exports, "", "void warn(?&in)", asFUNCTION(warnAny), asCALL_GENERIC);

	/*
	!CompatMeta
	for     Global
	with    void error(?&in)
	as      void error(?&in)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void errorAny(asIScriptGeneric* Gen)
	{
		std::string Message = ValueToString(Gen->GetEngine(), Gen->GetArgAddress(0), Gen->GetArgTypeId(0));
		std::u8string Str(Message.begin(), Message.end());
		error(Str);
	}
	GECKO_EXPORT_FN_EX(s_Exports, "", "void error(?&in)", asFUNCTION(errorAny), asCALL_GENERIC);

	static void CopyMemoryBuffer(MemoryBuffer& Dst, const MemoryBuffer& Src)
	{
		if (&Dst == &Src) return;
		Dst.Resize(Src.GetSize());
		Dst.Seek(0);
		Dst.Write(Src, Src.GetSize());
		Dst.Seek(Src.GetPosition());
	}

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer@ MemoryBuffer()
	as      MemoryBuffer@ MemoryBuffer()
	being   behavior
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer* MemoryBuffer_Factory()
	{
		return new MemoryBuffer();
	}
	GLOBAL_MEMORYBUFFER_BEHAVIOUR(asBEHAVE_FACTORY, "MemoryBuffer@+ f()", asFUNCTION(MemoryBuffer_Factory), asCALL_CDECL);

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer@ MemoryBuffer(const MemoryBuffer&in)
	as      MemoryBuffer@ MemoryBuffer(const MemoryBuffer&in)
	being   behavior
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer* MemoryBuffer_FactoryCopy(const MemoryBuffer& Other)
	{
		MemoryBuffer* Buf = new MemoryBuffer();
		CopyMemoryBuffer(*Buf, Other);
		return Buf;
	}
	GLOBAL_MEMORYBUFFER_BEHAVIOUR(asBEHAVE_FACTORY, "MemoryBuffer@+ f(const MemoryBuffer&in)", asFUNCTION(MemoryBuffer_FactoryCopy), asCALL_CDECL);

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer@ MemoryBuffer(uint64 size)
	as      MemoryBuffer@ MemoryBuffer(uint64 size)
	being   behavior
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer* MemoryBuffer_FactorySize(uint64_t Size)
	{
		return new MemoryBuffer(Size);
	}
	GLOBAL_MEMORYBUFFER_BEHAVIOUR(asBEHAVE_FACTORY, "MemoryBuffer@+ f(uint64 size)", asFUNCTION(MemoryBuffer_FactorySize), asCALL_CDECL);

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer@ MemoryBuffer(uint64 size, uint8 init)
	as      MemoryBuffer@ MemoryBuffer(uint64 size, uint8 init)
	being   behavior
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer* MemoryBuffer_FactorySizeInit(uint64_t Size, uint8_t Init)
	{
		return new MemoryBuffer(Size, Init);
	}
	GLOBAL_MEMORYBUFFER_BEHAVIOUR(asBEHAVE_FACTORY, "MemoryBuffer@+ f(uint64 size, uint8 init)", asFUNCTION(MemoryBuffer_FactorySizeInit), asCALL_CDECL);

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer& opAssign(const MemoryBuffer&in)
	as      MemoryBuffer& opAssign(const MemoryBuffer&in)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer& MemoryBuffer_opAssign(const MemoryBuffer& Other, MemoryBuffer* Buf)
	{
		CopyMemoryBuffer(*Buf, Other);
		return *Buf;
	}
	GLOBAL_MEMORYBUFFER_METHOD("MemoryBuffer& opAssign(const MemoryBuffer&in)", asFUNCTION(MemoryBuffer_opAssign), asCALL_CDECL_OBJLAST);

	/*
	!CompatMeta
	for     Global
	with    void Resize(uint64 size)
	as      void Resize(uint64 size)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Resize(uint64 size)", asMETHOD(MemoryBuffer, Resize), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint64 GetSize() const
	as      uint64 GetSize() const
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint64 GetSize()", asMETHOD(MemoryBuffer, GetSize), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint64 GetPosition() const
	as      uint64 GetPosition() const
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint64 GetPosition()", asMETHOD(MemoryBuffer, GetPosition), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Seek(uint64 pos, int dir = 0)
	as      void Seek(uint64 pos, int dir = 0)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static void MemoryBuffer_Seek(uint64_t Pos, int Dir, MemoryBuffer* Buf)
	{
		Buf->Seek(Pos, (SeekingDirection)Dir);
	}
	GLOBAL_MEMORYBUFFER_METHOD("void Seek(uint64 pos, int dir = 0)", asFUNCTION(MemoryBuffer_Seek), asCALL_CDECL_OBJLAST);

	/*
	!CompatMeta
	for     Global
	with    bool AtEnd() const
	as      bool AtEnd() const
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("bool AtEnd()", asMETHOD(MemoryBuffer, AtEnd), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const uint8&in i)
	as      void Write(const uint8&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const uint8&in i)", asMETHODPR(MemoryBuffer, Write, (const uint8_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const uint16&in i)
	as      void Write(const uint16&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const uint16&in i)", asMETHODPR(MemoryBuffer, Write, (const uint16_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const uint&in i)
	as      void Write(const uint&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const uint&in i)", asMETHODPR(MemoryBuffer, Write, (const uint32_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const uint64&in i)
	as      void Write(const uint64&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const uint64&in i)", asMETHODPR(MemoryBuffer, Write, (const uint64_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const int8&in i)
	as      void Write(const int8&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const int8&in i)", asMETHODPR(MemoryBuffer, Write, (const int8_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const int16&in i)
	as      void Write(const int16&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const int16&in i)", asMETHODPR(MemoryBuffer, Write, (const int16_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const int&in i)
	as      void Write(const int&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const int&in i)", asMETHODPR(MemoryBuffer, Write, (const int32_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const int64&in i)
	as      void Write(const int64&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const int64&in i)", asMETHODPR(MemoryBuffer, Write, (const int64_t&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const float&in i)
	as      void Write(const float&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const float&in i)", asMETHODPR(MemoryBuffer, Write, (const float&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const double&in i)
	as      void Write(const double&in i)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const double&in i)", asMETHODPR(MemoryBuffer, Write, (const double&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void Write(const string&in str)
	as      void Write(const string&in str)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void Write(const string&in str)", asMETHODPR(MemoryBuffer, Write, (const std::u8string&), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void WriteFromBuffer(const MemoryBuffer&in buf, uint64 size)
	as      void WriteFromBuffer(const MemoryBuffer&in buf, uint64 size)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void WriteFromBuffer(const MemoryBuffer&in buf, uint64 size)", asMETHODPR(MemoryBuffer, Write, (const MemoryBuffer&, uint64_t), void), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint8 ReadUInt8()
	as      uint8 ReadUInt8()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint8 ReadUInt8()", asMETHOD(MemoryBuffer, ReadUInt8), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint16 ReadUInt16()
	as      uint16 ReadUInt16()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint16 ReadUInt16()", asMETHOD(MemoryBuffer, ReadUInt16), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint ReadUInt32()
	as      uint ReadUInt32()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint ReadUInt32()", asMETHOD(MemoryBuffer, ReadUInt32), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    uint64 ReadUInt64()
	as      uint64 ReadUInt64()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("uint64 ReadUInt64()", asMETHOD(MemoryBuffer, ReadUInt64), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    int8 ReadInt8()
	as      int8 ReadInt8()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("int8 ReadInt8()", asMETHOD(MemoryBuffer, ReadInt8), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    int16 ReadInt16()
	as      int16 ReadInt16()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("int16 ReadInt16()", asMETHOD(MemoryBuffer, ReadInt16), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    int ReadInt32()
	as      int ReadInt32()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("int ReadInt32()", asMETHOD(MemoryBuffer, ReadInt32), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    int64 ReadInt64()
	as      int64 ReadInt64()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("int64 ReadInt64()", asMETHOD(MemoryBuffer, ReadInt64), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    float ReadFloat()
	as      float ReadFloat()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("float ReadFloat()", asMETHOD(MemoryBuffer, ReadFloat), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    double ReadDouble()
	as      double ReadDouble()
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("double ReadDouble()", asMETHOD(MemoryBuffer, ReadDouble), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    string ReadString(uint64 size)
	as      string ReadString(uint64 size)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("string ReadString(uint64 size)", asMETHOD(MemoryBuffer, ReadString), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    MemoryBuffer@ ReadBuffer(uint64 size)
	as      MemoryBuffer@ ReadBuffer(uint64 size)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	static MemoryBuffer* MemoryBuffer_ReadBuffer(uint64_t Size, MemoryBuffer* Buf)
	{
		return new MemoryBuffer(Buf->ReadBuffer(Size));
	}
	GLOBAL_MEMORYBUFFER_METHOD("MemoryBuffer@+ ReadBuffer(uint64 size)", asFUNCTION(MemoryBuffer_ReadBuffer), asCALL_CDECL_OBJLAST);

	/*
	!CompatMeta
	for     Global
	with    void WriteFromBase64(const string&in, bool url = false)
	as      void WriteFromBase64(const string&in, bool url = false)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void WriteFromBase64(const string&in, bool url = false)", asMETHOD(MemoryBuffer, WriteFromBase64), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    string ReadToBase64(uint64 size, bool url = false)
	as      string ReadToBase64(uint64 size, bool url = false)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("string ReadToBase64(uint64 size, bool url = false)", asMETHOD(MemoryBuffer, ReadToBase64), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    void WriteFromHex(const string&in)
	as      void WriteFromHex(const string&in)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("void WriteFromHex(const string&in)", asMETHOD(MemoryBuffer, WriteFromHex), asCALL_THISCALL);

	/*
	!CompatMeta
	for     Global
	with    string ReadToHex(uint64 size, bool upper = false)
	as      string ReadToHex(uint64 size, bool upper = false)
	being   method
	of      MemoryBuffer
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	GLOBAL_MEMORYBUFFER_METHOD("string ReadToHex(uint64 size, bool upper = false)", asMETHOD(MemoryBuffer, ReadToHex), asCALL_THISCALL);

	void RegisterBaseTypes(asIScriptEngine* Engine)
	{
		/*
		!CompatMeta
		for     Global
		with    ~string()
		with    string()
		with    string(const string&in)
		being   behavior
		of      string
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		/*
		!CompatMeta
		for     Global
		with    int opCmp(const string&in) const
		with    uint8& opIndex(uint)
		with    const uint8& opIndex(uint) const
		with    string& opAssign(const string&in)
		with    string& opAddAssign(const string&in)
		with    string opAdd(const string&in) const
		with    string opAdd(int64) const
		with    string opAdd_r(int64) const
		with    string opAdd(uint64) const
		with    string opAdd_r(uint64) const
		with    string opAdd(float) const
		with    string opAdd_r(float) const
		with    string opAdd(bool) const
		with    string opAdd_r(bool) const
		being   method
		of      string
		declare full
		since   v0.1.0
		!CompatMeta
		*/
		RegisterStdString(Engine);
		RegisterScriptArray(Engine, true);
		RegisterCoroutines(Engine);
	}

	void Registrar(asIScriptEngine* Engine)
	{
		Engine->RegisterObjectType("MemoryBuffer", sizeof(MemoryBuffer), asOBJ_REF);

		/*
		!CompatMeta
		for     Global
		with    void $beh5()
		as      void $beh5()
		being   behavior
		of      MemoryBuffer
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectBehaviour("MemoryBuffer", asBEHAVE_ADDREF, "void f()", asMETHOD(MemoryBuffer, AddRef), asCALL_THISCALL);

		/*
		!CompatMeta
		for     Global
		with    void $beh6()
		as      void $beh6()
		being   behavior
		of      MemoryBuffer
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		Engine->RegisterObjectBehaviour("MemoryBuffer", asBEHAVE_RELEASE, "void f()", asMETHOD(MemoryBuffer, RemoveRef), asCALL_THISCALL);

		RegisterAll(Engine, s_Exports);
	}
}
