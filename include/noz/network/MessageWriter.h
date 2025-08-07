#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace noz::network
{
    class MessageWriter
    {
    private:
        std::vector<uint8_t>& _data;

    public:
        explicit MessageWriter(std::vector<uint8_t>& data);
        
        // Basic types
        void writeBool(bool value);
        void writeInt8(int8_t value);
        void writeUInt8(uint8_t value);
        void writeInt16(int16_t value);
        void writeUInt16(uint16_t value);
        void writeInt32(int32_t value);
        void writeUInt32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUInt64(uint64_t value);
        void writeFloat(float value);
        void writeDouble(double value);
        
        // String
        void writeString(const std::string& value);
        
        // Vector types
        void writeBytes(const std::vector<uint8_t>& bytes);
        void writeBytes(const uint8_t* data, size_t size);
        
        // Utility
        size_t size() const;
        void clear();
    };
} 