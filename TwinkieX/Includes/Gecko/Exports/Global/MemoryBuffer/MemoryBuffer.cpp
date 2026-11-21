#include "pch.h"
#include "MemoryBuffer.h"

MemoryBuffer::MemoryBuffer(const void* data, uint64_t size)
{
    Resize(size);
    if (size) memcpy(Data, data, size);
}

MemoryBuffer::MemoryBuffer(uint64_t size, uint8_t init)
{
    Resize(size);
    if (size) memset(Data, init, size);
}

MemoryBuffer::MemoryBuffer(const MemoryBuffer& Other)
    : MemoryBuffer(Other.Data, Other.Size)
{
    Cursor = Other.Cursor;
}

MemoryBuffer::MemoryBuffer(MemoryBuffer&& Other) noexcept
    : Data(Other.Data), Size(Other.Size), Cursor(Other.Cursor)
{
    Other.Data = nullptr;
    Other.Size = 0;
    Other.Cursor = 0;
}

MemoryBuffer& MemoryBuffer::operator=(const MemoryBuffer& Other)
{
    if (this == &Other) return *this;

    Resize(Other.Size);
    if (Size) memcpy(Data, Other.Data, Size);
    Cursor = Other.Cursor;
    return *this;
}

MemoryBuffer::~MemoryBuffer()
{
    free(Data);
}

void MemoryBuffer::AddRef()
{
    RefCount++;
}

void MemoryBuffer::RemoveRef()
{
    if (!--RefCount) delete this;
}

// New bytes are zeroed
void MemoryBuffer::Resize(uint64_t size)
{
    if (size == Size) return;
    if (size > SIZE_MAX) return;

    if (size == 0)
    {
        free(Data);
        Data = nullptr;
    }
    else
    {
        uint8_t* NewData = (uint8_t*)realloc(Data, size);
        if (!NewData) return;
        if (size > Size) memset(NewData + Size, 0, size - Size);
        Data = NewData;
    }

    Size = size;
    Cursor = Cursor > Size ? Size : Cursor;
}

uint64_t MemoryBuffer::GetSize() const
{
    return Size;
}

uint64_t MemoryBuffer::GetPosition() const
{
    return Cursor;
}

void MemoryBuffer::Seek(uint64_t pos, SeekingDirection dir)
{
    switch (dir)
    {
    case SeekingDirection::FromStart:
        Cursor = pos;
        break;
    case SeekingDirection::Forwards:
        Cursor = pos > Size - Cursor ? Size : Cursor + pos;
        break;
    case SeekingDirection::Backwards:
        Cursor = pos <= Cursor ? Cursor - pos : 0;
        break;
    }

    Cursor = Cursor > Size ? Size : Cursor;
}

bool MemoryBuffer::AtEnd() const
{
    return Cursor >= Size;
}

uint64_t MemoryBuffer::Remaining() const
{
    return Size - Cursor;
}

bool MemoryBuffer::MakeRoom(uint64_t Count)
{
    if (Count <= Remaining()) return true;
    if (Count > UINT64_MAX - Cursor) return false;

    Resize(Cursor + Count);
    return Count <= Remaining();
}

void MemoryBuffer::WriteBytes(const void* Bytes, uint64_t Count)
{
    if (Count == 0 || !MakeRoom(Count)) return;
    memcpy(Data + Cursor, Bytes, Count);
    Cursor += Count;
}

#define MemoryBuffer_WriteFn(T) void MemoryBuffer::Write(const T& i) { WriteBytes(&i, sizeof(T)); }

MemoryBuffer_WriteFn(uint8_t);
MemoryBuffer_WriteFn(uint16_t);
MemoryBuffer_WriteFn(uint32_t);
MemoryBuffer_WriteFn(uint64_t);
MemoryBuffer_WriteFn(int8_t);
MemoryBuffer_WriteFn(int16_t);
MemoryBuffer_WriteFn(int32_t);
MemoryBuffer_WriteFn(int64_t);
MemoryBuffer_WriteFn(float);
MemoryBuffer_WriteFn(double);

void MemoryBuffer::Write(const std::u8string& str)
{
    WriteBytes(str.data(), str.size());
}

// Copies from the start of buf
void MemoryBuffer::Write(const MemoryBuffer& buf, uint64_t size)
{
    if (size > buf.Size) size = buf.Size;

    // buf can be this buffer, which growing would move
    std::vector<uint8_t> Bytes(buf.Data, buf.Data + size);
    WriteBytes(Bytes.data(), Bytes.size());
}

static const char* Base64Alphabet(bool url)
{
    return url ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
               : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

void MemoryBuffer::WriteFromBase64(const std::u8string& str, bool url)
{
    int Values[256];
    std::fill(std::begin(Values), std::end(Values), -1);
    const char* Alphabet = Base64Alphabet(url);
    for (int i = 0; i < 64; i++) Values[(unsigned char)Alphabet[i]] = i;

    std::u8string_view Input = str;
    while (!Input.empty() && Input.back() == '=') Input.remove_suffix(1);
    if (Input.size() % 4 == 1 || str.size() - Input.size() > 2) return;

    std::vector<uint8_t> Bytes;
    Bytes.reserve(Input.size() * 3 / 4);

    uint32_t Bits = 0;
    int BitCount = 0;
    for (char8_t c : Input)
    {
        int Value = Values[(unsigned char)c];
        if (Value < 0) return;

        Bits = (Bits << 6) | (uint32_t)Value;
        BitCount += 6;
        if (BitCount >= 8)
        {
            BitCount -= 8;
            Bytes.push_back((uint8_t)(Bits >> BitCount));
        }
    }

    WriteBytes(Bytes.data(), Bytes.size());
}

static int HexCharToInt(char8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void MemoryBuffer::WriteFromHex(const std::u8string& str)
{
    if (str.size() % 2 != 0) return;

    std::vector<uint8_t> Bytes;
    Bytes.reserve(str.size() / 2);
    for (size_t i = 0; i < str.size(); i += 2)
    {
        int High = HexCharToInt(str[i]);
        int Low = HexCharToInt(str[i + 1]);
        if (High < 0 || Low < 0) return;
        Bytes.push_back((uint8_t)((High << 4) | Low));
    }

    WriteBytes(Bytes.data(), Bytes.size());
}

#define MemoryBuffer_ReadFn(T, Name) T MemoryBuffer::Read##Name() \
{ \
    T Value{}; \
    if (sizeof(T) > Remaining()) return Value; \
    memcpy(&Value, Data + Cursor, sizeof(T)); \
    Cursor += sizeof(T); \
    return Value; \
}

MemoryBuffer_ReadFn(uint8_t, UInt8);
MemoryBuffer_ReadFn(uint16_t, UInt16);
MemoryBuffer_ReadFn(uint32_t, UInt32);
MemoryBuffer_ReadFn(uint64_t, UInt64);
MemoryBuffer_ReadFn(int8_t, Int8);
MemoryBuffer_ReadFn(int16_t, Int16);
MemoryBuffer_ReadFn(int32_t, Int32);
MemoryBuffer_ReadFn(int64_t, Int64);
MemoryBuffer_ReadFn(float, Float);
MemoryBuffer_ReadFn(double, Double);

std::u8string MemoryBuffer::ReadString(uint64_t size)
{
    if (size > Remaining()) size = Remaining();

    std::u8string Result((const char8_t*)(Data + Cursor), size);
    Cursor += size;
    return Result;
}

MemoryBuffer MemoryBuffer::ReadBuffer(uint64_t size)
{
    if (size > Remaining()) size = Remaining();

    MemoryBuffer Result(Data + Cursor, size);
    Cursor += size;
    return Result;
}

std::u8string MemoryBuffer::ReadToBase64(uint64_t size, bool url)
{
    if (size > Remaining()) size = Remaining();

    const char* Alphabet = Base64Alphabet(url);
    std::u8string Result;
    Result.reserve((size + 2) / 3 * 4);

    for (uint64_t i = 0; i < size; i += 3)
    {
        uint64_t Count = size - i < 3 ? size - i : 3;
        uint32_t Triple = 0;
        for (uint64_t b = 0; b < 3; b++)
        {
            Triple = (Triple << 8) | (b < Count ? Data[Cursor + i + b] : 0);
        }

        for (uint64_t c = 0; c < 4; c++)
        {
            Result.push_back(c <= Count ? Alphabet[(Triple >> (18 - 6 * c)) & 0x3F] : '=');
        }
    }

    Cursor += size;
    return Result;
}

std::u8string MemoryBuffer::ReadToHex(uint64_t size, bool upper)
{
    if (size > Remaining()) size = Remaining();

    const char* HexDigits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    std::u8string Result;
    Result.reserve(size * 2);

    for (uint64_t i = 0; i < size; i++)
    {
        uint8_t B = Data[Cursor + i];
        Result.push_back(HexDigits[B >> 4]);
        Result.push_back(HexDigits[B & 0x0F]);
    }

    Cursor += size;
    return Result;
}
