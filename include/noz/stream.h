//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>
#include "string.h"

enum StreamEndianess {
    STREAM_ENDIANNESS_DEFAULT,
    STREAM_ENDIANNESS_LITTLE = STREAM_ENDIANNESS_DEFAULT,
    STREAM_ENDIANNESS_BIG
};

struct Stream {};

// @stream
extern Stream* CreateStream(Allocator* allocator, u32 capacity, u32 initial_size = 0);
extern Stream* CreateStream(Allocator* allocator, u8* data, u32 size);
extern Stream* LoadStream(Allocator* allocator, const u8* data, u32 size);
extern Stream* LoadStream(Allocator* allocator, const std::filesystem::path& path);

// @endian
extern void SetEndianness(Stream* stream, StreamEndianess endian);

// @file
bool SaveStream(Stream* stream, const std::filesystem::path& path);

// @data
extern u8* GetData(Stream* stream);
inline u8* GetDataAt(Stream* stream, u32 position) { return GetData(stream) + position; }
extern u32 GetSize(Stream* stream);
extern void Clear(Stream* stream);
extern void Resize(Stream* stream, u32 capcity);

// @position
u32 GetPosition(Stream* stream);
void SetPosition(Stream* stream, u32 position);
u32 SeekBegin(Stream* stream, u32 offset);
u32 SeekEnd(Stream* stream, u32 offset);
u32 SeekCurrent(Stream* stream, u32 offset);
bool IsEOS(Stream* stream);

// @read
extern u8 ReadU8(Stream* stream);
extern u16 ReadU16(Stream* stream);
extern u32 ReadU32(Stream* stream);
extern u64 ReadU64(Stream* stream);
extern i8 ReadI8(Stream* stream);
extern i16 ReadI16(Stream* stream);
extern i32 ReadI32(Stream* stream);
extern i64 ReadI64(Stream* stream);
extern float ReadFloat(Stream* stream);
extern double ReadDouble(Stream* stream);
extern bool ReadBool(Stream* stream);
extern const Name* ReadName(Stream* stream);
extern Color ReadColor(Stream* stream);
extern Vec3 ReadVec3(Stream* stream);
extern Vec2 ReadVec2(Stream* stream);
extern int ReadBytes(Stream* stream, void* dest, u32 count);
extern void AlignStream(Stream* stream, int alignment);

extern int ReadString(Stream* stream, char* buffer, int buffer_size);
inline int ReadString(Stream* stream, String64& dst) {
    return ReadString(stream, dst.value, sizeof(dst.value));
}
inline int ReadString(Stream* stream, String1024& dst) {
    return ReadString(stream, dst.value, sizeof(dst.value));
}

extern int ReadNullString(Stream* stream, char* buffer, int buffer_size);
inline int ReadNullString(Stream* stream, Text& text) {
    return ReadNullString(stream, text.value, sizeof(text.value));
}
inline int ReadNullString(Stream* stream, String64& text) {
    return ReadNullString(stream, text.value, 64);
}

template <typename TStruct> TStruct ReadStruct(Stream* stream)
{
    TStruct result;
    ReadBytes(stream, &result, sizeof(TStruct));
    return result;
}

// @write
extern void Copy(Stream* dst, Stream* src);
extern void Copy(Stream* dst, Stream* src, int count);
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
void WriteBytes(Stream* stream, const void* data, u32 size);
void WriteName(Stream* stream, const Name* name);

template <typename TStruct> void WriteStruct(Stream* stream, const TStruct& value)
{
    WriteBytes(stream, &value, sizeof(TStruct));
}
