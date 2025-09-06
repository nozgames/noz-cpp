//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace std 
{
    namespace filesystem 
    {
        class path;
    }
}

struct Stream {};

// @alloc
Stream* CreateStream(Allocator* allocator, size_t capacity);
Stream* LoadStream(Allocator* allocator, u8* data, size_t size);
Stream* LoadStream(Allocator* allocator, const std::filesystem::path& path);

// @file
bool SaveStream(Stream* stream, const std::filesystem::path& path);

// @data
u8* GetData(Stream* stream);
size_t GetSize(Stream* stream);
void Clear(Stream* stream);

// @position
size_t GetPosition(Stream* stream);
void SetPosition(Stream* stream, size_t position);
size_t SeekBegin(Stream* stream, size_t offset);
size_t SeekEnd(Stream* stream, size_t offset);
size_t SeekCurrent(Stream* stream, size_t offset);
bool IsEOS(Stream* stream);

// @read
bool ReadFileSignature(Stream* stream, const char* expected_signature, size_t signature_length);
u8 ReadU8(Stream* stream);
u16 ReadU16(Stream* stream);
u32 ReadU32(Stream* stream);
u64 ReadU64(Stream* stream);
i8 ReadI8(Stream* stream);
i16 ReadI16(Stream* stream);
i32 ReadI32(Stream* stream);
i64 ReadI64(Stream* stream);
float ReadFloat(Stream* stream);
double ReadDouble(Stream* stream);
bool ReadBool(Stream* stream);
int ReadString(Stream* stream, char* buffer, int buffer_size);
const Name* ReadName(Stream* stream);
Color ReadColor(Stream* stream);
Vec3 ReadVec3(Stream* stream);
Vec2 ReadVec2(Stream* stream);
Rect ReadRect(Stream* stream);
void ReadBytes(Stream* stream, void* dest, size_t count);

template <typename TStruct> TStruct ReadStruct(Stream* stream)
{
    TStruct result;
    ReadBytes(stream, &result, sizeof(TStruct));
    return result;
}

// @write
void WriteFileSignature(Stream* stream, const char* signature, size_t signature_length);
void WriteU8(Stream* stream, u8 value);
void WriteU16(Stream* stream, u16 value);
void WriteU32(Stream* stream, u32 value);
void WriteU64(Stream* stream, u64 value);
void WriteI8(Stream* stream, i8 value);
void WriteI16(Stream* stream, i16 value);
void WriteI32(Stream* stream, i32 value);
void WriteI64(Stream* stream, i64 value);
void WriteFloat(Stream* stream, float value);
void WriteDouble(Stream* stream, double value);
void WriteBool(Stream* stream, bool value);
void WriteString(Stream* stream, const char* value);
void WriteVec3(Stream* stream, const Vec3& value);
void WriteVec2(Stream* stream, const Vec2& value);
void WriteCSTR(Stream* stream, const char* format, ...); // Write formatted C string without length prefix
void WriteColor(Stream* stream, Color value);
void WriteRect(Stream* stream, const Rect& value);
void WriteBytes(Stream* stream, const void* data, size_t size);

template <typename TStruct> void WriteStruct(Stream* stream, const TStruct& value)
{
    WriteBytes(stream, &value, sizeof(TStruct));
}
