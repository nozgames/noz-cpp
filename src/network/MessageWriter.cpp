/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/network/MessageWriter.h>

namespace noz::network
{
    MessageWriter::MessageWriter(std::vector<uint8_t>& data)
        : _data(data)
    {
    }

    void MessageWriter::writeBool(bool value)
    {
        _data.push_back(value ? 1 : 0);
    }

    void MessageWriter::writeInt8(int8_t value)
    {
        _data.push_back(static_cast<uint8_t>(value));
    }

    void MessageWriter::writeUInt8(uint8_t value)
    {
        _data.push_back(value);
    }

    void MessageWriter::writeInt16(int16_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(int16_t));
        std::memcpy(&_data[oldSize], &value, sizeof(int16_t));
    }

    void MessageWriter::writeUInt16(uint16_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(uint16_t));
        std::memcpy(&_data[oldSize], &value, sizeof(uint16_t));
    }

    void MessageWriter::writeInt32(int32_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(int32_t));
        std::memcpy(&_data[oldSize], &value, sizeof(int32_t));
    }

    void MessageWriter::writeUInt32(uint32_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(uint32_t));
        std::memcpy(&_data[oldSize], &value, sizeof(uint32_t));
    }

    void MessageWriter::writeInt64(int64_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(int64_t));
        std::memcpy(&_data[oldSize], &value, sizeof(int64_t));
    }

    void MessageWriter::writeUInt64(uint64_t value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(uint64_t));
        std::memcpy(&_data[oldSize], &value, sizeof(uint64_t));
    }

    void MessageWriter::writeFloat(float value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(float));
        std::memcpy(&_data[oldSize], &value, sizeof(float));
    }

    void MessageWriter::writeDouble(double value)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + sizeof(double));
        std::memcpy(&_data[oldSize], &value, sizeof(double));
    }

    void MessageWriter::writeString(const std::string& value)
    {
        writeUInt16(static_cast<uint32_t>(value.length()));
        size_t oldSize = _data.size();
        _data.resize(oldSize + value.length());
        std::memcpy(&_data[oldSize], value.data(), value.length());
    }

    void MessageWriter::writeBytes(const std::vector<uint8_t>& bytes)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + bytes.size());
        std::memcpy(&_data[oldSize], bytes.data(), bytes.size());
    }

    void MessageWriter::writeBytes(const uint8_t* data, size_t size)
    {
        size_t oldSize = _data.size();
        _data.resize(oldSize + size);
        std::memcpy(&_data[oldSize], data, size);
    }

    size_t MessageWriter::size() const
    {
        return _data.size();
    }

    void MessageWriter::clear()
    {
        _data.clear();
    }
} 