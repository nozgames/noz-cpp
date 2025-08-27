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

struct Stream : Object {};

// @alloc
Stream* CreateStream(Allocator* allocator, size_t capacity);
Stream* LoadStream(Allocator* allocator, uint8_t* data, size_t size);
Stream* LoadStream(Allocator* allocator, const std::filesystem::path& path);

// @file
bool SaveStream(Stream* stream, const std::filesystem::path& path);

// @data
uint8_t* GetData(Stream* stream);
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
uint8_t ReadU8(Stream* stream);
uint16_t ReadU16(Stream* stream);
uint32_t ReadU32(Stream* stream);
uint64_t ReadU64(Stream* stream);
int8_t ReadI8(Stream* stream);
int16_t ReadI16(Stream* stream);
int32_t ReadI32(Stream* stream);
int64_t ReadI64(Stream* stream);
float ReadFloat(Stream* stream);
double ReadDouble(Stream* stream);
bool ReadBool(Stream* stream);
void ReadBytes(Stream* stream, void* dest, size_t count);
int ReadString(Stream* stream, char* buffer, int buffer_size);
color_t ReadColor(Stream* stream);

// @write
void WriteFileSignature(Stream* stream, const char* signature, size_t signature_length);
void WriteU8(Stream* stream, uint8_t value);
void WriteU16(Stream* stream, uint16_t value);
void WriteU32(Stream* stream, uint32_t value);
void WriteU64(Stream* stream, uint64_t value);
void WriteI8(Stream* stream, int8_t value);
void WriteI16(Stream* stream, int16_t value);
void WriteI32(Stream* stream, int32_t value);
void WriteI64(Stream* stream, int64_t value);
void WriteFloat(Stream* stream, float value);
void WriteDouble(Stream* stream, double value);
void WriteBool(Stream* stream, bool value);
void WriteString(Stream* stream, const char* value);
void WriteCSTR(Stream* stream, const char* format, ...); // Write formatted C string without length prefix
void WriteBytes(Stream* stream, void* data, size_t size);
void WriteColor(Stream* stream, color_t value);


#define ReadStruct(stream, type) \
    ({ type result; stream_read(stream, &result, sizeof(type)); result; })

#define WriteStruct(stream, value) \
    stream_write(stream, &(value), sizeof(value))