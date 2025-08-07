#pragma once

namespace noz::network
{
    class MessageReader
    {
    private:
        const std::vector<uint8_t>& _data;
        size_t _position;

    public:
        explicit MessageReader(const std::vector<uint8_t>& data);
        
        // Basic types
        bool readBool();
        int8_t readInt8();
        uint8_t readUInt8();
        int16_t readInt16();
        uint16_t readUInt16();
        int32_t readInt32();
        uint32_t readUInt32();
        int64_t readInt64();
        uint64_t readUInt64();
        float readFloat();
        double readDouble();
        
        // String
        std::string readString();
        
        // Vector types
        void readBytes(std::vector<uint8_t>& bytes, size_t count);
        
        // Utility
        size_t remainingBytes() const;
        bool hasMoreData() const;
        void reset();
    };
} 