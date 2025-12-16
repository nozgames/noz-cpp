//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <stdio.h>
#include <stdarg.h>
#include <filesystem>

#define DEFAULT_INITIAL_CAPACITY 256

struct StreamImpl : Stream {
    u8* data;
    u32 size;
    u32 capacity;
    u32 position;
    bool free_data;
    StreamEndianess endianess;
};

static void EnsureCapacity(StreamImpl* impl, u32 required_size);

void StreamDestructor(void* s) {
    StreamImpl* impl = static_cast<StreamImpl *>(s);
    if (impl->free_data)
        Free(impl->data);
}

void SetEndianness(Stream* stream, StreamEndianess endian) {
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    impl->endianess = endian;
}

Stream* CreateStream(Allocator* allocator, u32 capacity, u32 initial_size) {
    StreamImpl* impl = static_cast<StreamImpl *>(Alloc(allocator, sizeof(StreamImpl), StreamDestructor));

    if (capacity == 0)
        capacity = DEFAULT_INITIAL_CAPACITY;

    impl->data = static_cast<u8 *>(Alloc(allocator, capacity));
    impl->size = initial_size;
    impl->capacity = capacity;
    impl->position = 0;
    impl->free_data = true;
    
    return impl;
}

Stream* CreateStream(Allocator* allocator, u8* data, u32 size) {
    assert(data);
    assert(size > 0);

    StreamImpl* impl = static_cast<StreamImpl *>(Alloc(allocator, sizeof(StreamImpl), StreamDestructor));
    impl->capacity = size;
    impl->data = data;
    impl->size = size;
    impl->position = 0;
    return impl;
}

Stream* LoadStream(Allocator* allocator, const u8* data, u32 size) {
    StreamImpl* impl = static_cast<StreamImpl*>(CreateStream(allocator, size));
    EnsureCapacity(impl, size);
    memcpy(impl->data, data, size);
    impl->size = size;
    impl->position = 0;
    impl->free_data = true;
    return impl;
}

Stream* LoadStream(Allocator* allocator, const std::filesystem::path& path) {
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) return nullptr;
    
    fseek(file, 0, SEEK_END);
    u32 file_size = static_cast<u32>(ftell(file));
    fseek(file, 0, SEEK_SET);
    
    if (file_size == 0) {
        fclose(file);
        return nullptr;
    }
    
    StreamImpl* impl = static_cast<StreamImpl *>(CreateStream(allocator, file_size + 1));
    impl->free_data = true;
    EnsureCapacity(impl, file_size);
    impl->size = static_cast<u32>(fread(impl->data, 1, file_size, file));
    fclose(file);

    return impl;
}

bool SaveStream(Stream* stream, const std::filesystem::path& path) {
    if (!stream)
        return false;

    std::filesystem::path full_path = path.is_relative() ?  std::filesystem::current_path() / path : path;

    try
    {
        std::filesystem::create_directories(full_path.parent_path());
    }
    catch (...)
    {
    }

    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    FILE* file = fopen(full_path.string().c_str(), "wb");
    if (!file)
        return false;
    
    u32 bytes_written = (u32)fwrite(impl->data, 1, impl->size, file);
    fclose(file);
    
    return bytes_written == impl->size;
}

u8* GetData(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->data;
}

u32 GetSize(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->size;
}

void Clear(Stream* stream)
{
	StreamImpl* impl = static_cast<StreamImpl*>(stream);
    impl->size = 0;
    impl->position = 0;
}

u32 GetPosition(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->position;
}

void SetPosition(Stream* stream, u32 position)
{
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    impl->position = position;
    assert(impl->position <= impl->size);
}

u32 SeekBegin(Stream* stream, u32 offset)
{
    SetPosition(stream, offset);
    return GetPosition(stream);
}

u32 SeekCurrent(Stream* stream, u32 offset)
{
    SetPosition(stream, static_cast<StreamImpl*>(stream)->position + offset);
    return GetPosition(stream);
}

u32 SeekEnd(Stream* stream, u32 offset)
{
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    SetPosition(stream, Max(i32(impl->size - offset), 0));
    return impl->position;
}

bool IsEOS(Stream* stream)
{
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    return impl->position >= impl->size;
}

const Name* ReadName(Stream* stream)
{
    char name[256];
    ReadString(stream, name, sizeof(name));
    return GetName(name);
}

int ReadString(Stream* stream, char* buffer, int buffer_size)
{
    auto len = (int)ReadU32(stream);
    if (len == 0)
    {
        buffer[0] = 0;
        return len;
    }

    auto truncated_len = Min(buffer_size-1, len);
    ReadBytes(stream, buffer, truncated_len);
    buffer[truncated_len] = 0;

    if (truncated_len < len)
        SeekCurrent(stream, len - truncated_len);

    return truncated_len;
}

u8 ReadU8(Stream* stream) {
    if (!stream) return 0;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    if (impl->position + sizeof(u8) > impl->size)
        return 0;
    
    u8 value = impl->data[impl->position];
    impl->position += sizeof(u8);
    assert(impl->position <= impl->size);
    return value;
}

u16 ReadU16(Stream* stream) {
    uint16_t value;
    ReadBytes(stream, &value, sizeof(u16));

    if (static_cast<StreamImpl*>(stream)->endianess == STREAM_ENDIANNESS_BIG)
        value = ((value & 0x00FF) << 8) |
                ((value & 0xFF00) >> 8);

    return value;
}

u64 ReadU64BigEndian(Stream* stream) {
    uint64_t value;
    ReadBytes(stream, &value, sizeof(u64));

    return value;
}

u32 ReadU32(Stream* stream) {
    u32 value;
    ReadBytes(stream, &value, sizeof(u32));

    if (static_cast<StreamImpl*>(stream)->endianess == STREAM_ENDIANNESS_BIG)
        value = ((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) << 8)  |
            ((value & 0x00FF0000) >> 8)  |
            ((value & 0xFF000000) >> 24);

    return value;
}

uint64_t ReadU64(Stream* stream) {
    uint64_t value;
    ReadBytes(stream, &value, sizeof(uint64_t));

    if (static_cast<StreamImpl*>(stream)->endianess == STREAM_ENDIANNESS_BIG)
        value = ((value & 0x00000000000000FFULL) << 56) |
            ((value & 0x000000000000FF00ULL) << 40) |
            ((value & 0x0000000000FF0000ULL) << 24) |
            ((value & 0x00000000FF000000ULL) << 8)  |
            ((value & 0x000000FF00000000ULL) >> 8)  |
            ((value & 0x0000FF0000000000ULL) >> 24) |
            ((value & 0x00FF000000000000ULL) >> 40) |
            ((value & 0xFF00000000000000ULL) >> 56);

    return value;
}

i8 ReadI8(Stream* stream) {
    return static_cast<i8>(ReadU8(stream));
}

i16 ReadI16(Stream* stream) {
    return static_cast<i16>(ReadU16(stream));
}

i32 ReadI32(Stream* stream) {
    return static_cast<int32_t>(ReadU32(stream));
}

i64 ReadI64(Stream* stream) {
    return static_cast<int64_t>(ReadU64(stream));
}

f32 ReadFloat(Stream* stream) {
    float value;
    ReadBytes(stream, &value, sizeof(float));
    return value;
}

Vec2 ReadVec2(Stream* stream) {
    Vec2 value;
    ReadBytes(stream, &value, sizeof(Vec2));
    return value;
}

Vec3 ReadVec3(Stream* stream) {
    Vec3 value;
    ReadBytes(stream, &value, sizeof(Vec3));
    return value;
}

f64 ReadDouble(Stream* stream) {
    f64 value;
    ReadBytes(stream, &value, sizeof(f64));
    return value;
}

bool ReadBool(Stream* stream) {
    return ReadU8(stream) != 0;
}

int ReadBytes(Stream* stream, void* dest, u32 size) {
    if (!stream || !dest || size == 0) return 0;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    if (impl->position + size > impl->size) {
        // Read what we can and zero the rest
        u32 available = impl->size - impl->position;
        if (available > 0) 
        {
            memcpy(dest, impl->data + impl->position, available);
            impl->position += available;
            assert(impl->position <= impl->size);
        }
        // Zero remaining bytes
        if (size > available)
            memset(static_cast<u8 *>(dest) + available, 0, size - available);

        return static_cast<int>(available);
    }
    
    memcpy(dest, impl->data + impl->position, size);
    impl->position += size;
    assert(impl->position <= impl->size);

    return static_cast<int>(size);
}

void WriteU8(Stream* stream, u8 value) {
    WriteBytes(stream, &value, sizeof(u8));
}

void WriteU16(Stream* stream, uint16_t value)
{
    WriteBytes(stream, &value, sizeof(uint16_t));
}

void WriteU32(Stream* stream, uint32_t value)
{
    WriteBytes(stream, &value, sizeof(uint32_t));
}

void WriteU64(Stream* stream, uint64_t value)
{
    WriteBytes(stream, &value, sizeof(uint64_t));
}

void WriteI8(Stream* stream, int8_t value)
{
    WriteU8(stream, (u8)value);
}

void WriteI16(Stream* stream, int16_t value)
{
    WriteU16(stream, (uint16_t)value);
}

void WriteI32(Stream* stream, int32_t value)
{
    WriteU32(stream, (uint32_t)value);
}

void WriteI64(Stream* stream, int64_t value)
{
    WriteU64(stream, (uint64_t)value);
}

void WriteFloat(Stream* stream, float value)
{
    WriteBytes(stream, &value, sizeof(float));
}

void WriteVec3(Stream* stream, const Vec3& value)
{
    WriteBytes(stream, (void*)&value, sizeof(Vec3));
}

void WriteVec2(Stream* stream, const Vec2& value)
{
    WriteBytes(stream, &value, sizeof(Vec2));
}

void WriteDouble(Stream* stream, double value)
{
    WriteBytes(stream, &value, sizeof(double));
}

void WriteBool(Stream* stream, bool value)
{
    WriteU8(stream, value ? 1 : 0);
}

void WriteName(Stream* stream, const Name* name) {
    if (name == nullptr)
        WriteString(stream, "");
    else
        WriteString(stream, name->value);
}

void WriteString(Stream* stream, const char* value)
{
    if (!stream) return;
    
    if (!value) 
    {
        WriteU32(stream, 0);
        return;
    }
    
    u32 length = Length(value);
    WriteU32(stream, (uint32_t)length);
    WriteBytes(stream, value, length);
}

void WriteCSTR(Stream* stream, const char* format, ...)
{
    if (!stream || !format) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (written > 0 && (size_t)written < sizeof(buffer))
    {
        WriteBytes(stream, (u8*)buffer, written);
    }
}

void WriteBytes(Stream* stream, const void* data, u32 size)
{
    if (!stream || !data || size == 0) return;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    EnsureCapacity(impl, impl->position + size);
    
    memcpy(impl->data + impl->position, data, size);
    impl->position += size;
    
    // Update size if we've written past the current end
    if (impl->position > impl->size) 
        impl->size = impl->position;
}

// Color operations
Color ReadColor(Stream* stream)
{
    Color color = {0.0f, 0.0f, 0.0f, 1.0f};
    ReadBytes(stream, &color, sizeof(Color));
    return color;
}

void WriteColor(Stream* stream, Color value)
{
    WriteBytes(stream, &value, sizeof(Color));
}


static void Resize(StreamImpl* impl, u32 new_capacity) {
    if (new_capacity < impl->capacity)
        return;

    u8* new_data = static_cast<u8*>(Realloc(impl->data, new_capacity));
    if (!new_data)
        return;

    impl->data = new_data;
    impl->capacity = new_capacity;
}

static void EnsureCapacity(StreamImpl* impl, u32 required_size) {
    if (required_size <= impl->capacity) return;
    u32 new_capacity = impl->capacity;
    while (new_capacity < required_size) 
        new_capacity *= 2;

    Resize(impl, new_capacity);
}

int ReadNullString(Stream* stream, char* buffer, int buffer_size) {
    int count = 0;
    while (count < buffer_size - 1) {
        char c = static_cast<char>(ReadU8(stream));
        if (c == 0) break;
        buffer[count++] = c;
    }
    buffer[count] = 0;
    return count;
}

void AlignStream(Stream* stream, int alignment) {
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    const u32 pos = impl->position;
    const u32 mod = pos % alignment;
    if (mod != 0)
        impl->position += alignment - mod;
}

void Copy(Stream* dst, Stream* src) {
    if (!dst || !src) return;

    StreamImpl* dst_impl = static_cast<StreamImpl*>(dst);
    StreamImpl* src_impl = static_cast<StreamImpl*>(src);

    EnsureCapacity(dst_impl, dst_impl->position + src_impl->size);
    ReadBytes(src, dst_impl->data + dst_impl->position, src_impl->size);
    dst_impl->position += src_impl->size;
    dst_impl->size += src_impl->size;
}

void Copy(Stream* dst, Stream* src, int count) {
    if (!dst || !src) return;

    StreamImpl* dst_impl = static_cast<StreamImpl*>(dst);
    StreamImpl* src_impl = static_cast<StreamImpl*>(src);

    count = Min(count, static_cast<int>(src_impl->size - src_impl->position));

    EnsureCapacity(dst_impl, dst_impl->position + count);
    ReadBytes(src, dst_impl->data + dst_impl->position, count);
    dst_impl->position += count;
    dst_impl->size += count;
}

void Resize(Stream* stream, u32 new_size) {
    Resize(static_cast<StreamImpl*>(stream), new_size);
}
