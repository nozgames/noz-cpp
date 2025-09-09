//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <stdio.h>
#include <stdarg.h>
#include <filesystem>

#define DEFAULT_INITIAL_CAPACITY 256

struct StreamImpl : Stream
{
    u8* data;
    size_t size;
    size_t capacity;
    size_t position;
};

static void EnsureCapacity(StreamImpl* impl, size_t required_size);

void StreamDestructor(void* s)
{
    StreamImpl* impl = (StreamImpl*)s;
    Free(impl->data);
}

Stream* CreateStream(Allocator* allocator, size_t capacity)
{
    StreamImpl* impl = (StreamImpl*)Alloc(allocator, sizeof(StreamImpl), StreamDestructor);
    if (!impl)
        return nullptr;

    if (capacity == 0)
        capacity = DEFAULT_INITIAL_CAPACITY;

    impl->data = (u8*)Alloc(ALLOCATOR_DEFAULT, capacity);
    if (!impl->data) 
    {
        Free(impl);
        return nullptr;
    }
    
    impl->size = 0;
    impl->capacity = capacity;
    impl->position = 0;
    
    return impl;
}

Stream* LoadStream(Allocator* allocator, uint8_t* data, size_t size)
{
    StreamImpl* impl = (StreamImpl*)CreateStream(allocator, size);
    if (!impl)
        return nullptr;
        
    EnsureCapacity(impl, size);
    memcpy(impl->data, data, size);
    impl->size = size;
    impl->position = 0;
    
    return (Stream*)impl;
}

Stream* LoadStream(Allocator* allocator, const std::filesystem::path& path)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file)
        return nullptr;
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size == 0) 
    {
        fclose(file);
        return nullptr;
    }
    
    // Create stream and read file
    StreamImpl* impl = (StreamImpl*)CreateStream(allocator, file_size + 1);
    if (!impl)
    {
        fclose(file);
        return nullptr;
    }
    
    EnsureCapacity(impl, file_size);
    size_t bytes_read = fread(impl->data, 1, file_size, file);
    fclose(file);
    
    impl->size = bytes_read;
    impl->position = 0;
    
    return impl;
}

// todo: destructor
#if 0
void stream_destroy(Stream* stream) 
{
	StreamImpl* impl = static_cast<StreamImpl*>(stream);
    Destroy((Object*)stream);
}
#endif

bool SaveStream(Stream* stream, const std::filesystem::path& path)
{
    if (!stream)
        return false;

    std::filesystem::create_directory(path.parent_path());

    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file)
        return false;
    
    size_t bytes_written = fwrite(impl->data, 1, impl->size, file);
    fclose(file);
    
    return bytes_written == impl->size;
}

uint8_t* GetData(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->data;
}

size_t GetSize(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->size;
}

void Clear(Stream* stream)
{
	StreamImpl* impl = static_cast<StreamImpl*>(stream);
    impl->size = 0;
    impl->position = 0;
}

size_t GetPosition(Stream* stream)
{
    return static_cast<StreamImpl*>(stream)->position;
}

void SetPosition(Stream* stream, size_t position)
{
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    impl->position = position;
}

size_t SeekBegin(Stream* stream, size_t offset)
{
    SetPosition(stream, offset);
    return GetPosition(stream);
}

size_t SeekCurrent(Stream* stream, size_t offset)
{
    SetPosition(stream, static_cast<StreamImpl*>(stream)->position + offset);
    return GetPosition(stream);
}

size_t SeekEnd(Stream* stream, size_t offset)
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

bool ReadFileSignature(Stream* stream, const char* expected_signature, size_t signature_length)
{
    if (!stream || !expected_signature) return false;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    if (impl->position + signature_length > impl->size) return false;
    
    bool match = memcmp(impl->data + impl->position, expected_signature, signature_length) == 0;
    if (match) 
    {
        impl->position += signature_length;
    }
    return match;
}

uint8_t ReadU8(Stream* stream)
{
    if (!stream) return 0;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    if (impl->position + sizeof(uint8_t) > impl->size) return 0;
    
    uint8_t value = impl->data[impl->position];
    impl->position += sizeof(uint8_t);
    return value;
}

uint16_t ReadU16(Stream* stream)
{
    uint16_t value;
    ReadBytes(stream, &value, sizeof(uint16_t));
    return value;
}

uint32_t ReadU32(Stream* stream)
{
    uint32_t value;
    ReadBytes(stream, &value, sizeof(uint32_t));
    return value;
}

uint64_t ReadU64(Stream* stream)
{
    uint64_t value;
    ReadBytes(stream, &value, sizeof(uint64_t));
    return value;
}

int8_t ReadI8(Stream* stream)
{
    return (int8_t)ReadU8(stream);
}

int16_t ReadI16(Stream* stream)
{
    return (int16_t)ReadU16(stream);
}

int32_t ReadI32(Stream* stream)
{
    return (int32_t)ReadU32(stream);
}

int64_t ReadI64(Stream* stream)
{
    return (int64_t)ReadU64(stream);
}

float ReadFloat(Stream* stream)
{
    float value;
    ReadBytes(stream, &value, sizeof(float));
    return value;
}

Vec2 ReadVec2(Stream* stream)
{
    Vec2 value;
    ReadBytes(stream, &value, sizeof(Vec2));
    return value;
}

Vec3 ReadVec3(Stream* stream)
{
    Vec3 value;
    ReadBytes(stream, &value, sizeof(Vec3));
    return value;
}

Rect ReadRect(Stream* stream)
{
    Rect value;
    ReadBytes(stream, &value, sizeof(Rect));
    return value;
}

double ReadDouble(Stream* stream)
{
    double value;
    ReadBytes(stream, &value, sizeof(double));
    return value;
}

bool ReadBool(Stream* stream)
{
    return ReadU8(stream) != 0;
}

void ReadBytes(Stream* stream, void* dest, size_t size)
{
    if (!stream || !dest || size == 0) return;
    
    StreamImpl* impl = static_cast<StreamImpl*>(stream);
    
    if (impl->position + size > impl->size) 
    {
        // Read what we can and zero the rest
        size_t available = impl->size - impl->position;
        if (available > 0) 
        {
            memcpy(dest, impl->data + impl->position, available);
            impl->position += available;
        }
        // Zero remaining bytes
        if (size > available) 
        {
            memset((uint8_t*)dest + available, 0, size - available);
        }
        return;
    }
    
    memcpy(dest, impl->data + impl->position, size);
    impl->position += size;
}

// Writing operations
void WriteFileSignature(Stream* stream, const char* signature, size_t signature_length)
{
    WriteBytes(stream, (uint8_t*)signature, signature_length);
}

void WriteU8(Stream* stream, uint8_t value)
{
    WriteBytes(stream, &value, sizeof(uint8_t));
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
    WriteU8(stream, (uint8_t)value);
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

void WriteRect(Stream* stream, const Rect& value)
{
    WriteBytes(stream, (void*)&value, sizeof(Rect));
}

void WriteDouble(Stream* stream, double value)
{
    WriteBytes(stream, &value, sizeof(double));
}

void WriteBool(Stream* stream, bool value)
{
    WriteU8(stream, value ? 1 : 0);
}

void WriteString(Stream* stream, const char* value)
{
    if (!stream) return;
    
    if (!value) 
    {
        WriteU32(stream, 0);
        return;
    }
    
    size_t length = strlen(value);
    WriteU32(stream, (uint32_t)length);
    WriteBytes(stream, (uint8_t*)value, length);
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
        WriteBytes(stream, (uint8_t*)buffer, written);
    }
}

void WriteBytes(Stream* stream, const void* data, size_t size)
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

// Internal helper functions
static void EnsureCapacity(StreamImpl* impl, size_t required_size)
{
    if (required_size <= impl->capacity) return;
    
    size_t new_capacity = impl->capacity;
    while (new_capacity < required_size) 
        new_capacity *= 2;

    u8* new_data = (u8*)Realloc(impl->data, new_capacity);
    if (!new_data)
        return;

    impl->data = new_data;
    impl->capacity = new_capacity;
}

