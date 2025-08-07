/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/network/MessageReader.h>

namespace noz::network
{
    MessageReader::MessageReader(const std::vector<uint8_t>& data)
        : _data(data), _position(0)
    {
    }

    bool MessageReader::readBool()
    {
        if (_position >= _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        return _data[_position++] != 0;
    }

    int8_t MessageReader::readInt8()
    {
        if (_position >= _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        return static_cast<int8_t>(_data[_position++]);
    }

    uint8_t MessageReader::readUInt8()
    {
        if (_position >= _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        return _data[_position++];
    }

    int16_t MessageReader::readInt16()
    {
        if (_position + sizeof(int16_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        int16_t value;
        std::memcpy(&value, &_data[_position], sizeof(int16_t));
        _position += sizeof(int16_t);
        return value;
    }

    uint16_t MessageReader::readUInt16()
    {
        if (_position + sizeof(uint16_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        uint16_t value;
        std::memcpy(&value, &_data[_position], sizeof(uint16_t));
        _position += sizeof(uint16_t);
        return value;
    }

    int32_t MessageReader::readInt32()
    {
        if (_position + sizeof(int32_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        int32_t value;
        std::memcpy(&value, &_data[_position], sizeof(int32_t));
        _position += sizeof(int32_t);
        return value;
    }

    uint32_t MessageReader::readUInt32()
    {
        if (_position + sizeof(uint32_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        uint32_t value;
        std::memcpy(&value, &_data[_position], sizeof(uint32_t));
        _position += sizeof(uint32_t);
        return value;
    }

    int64_t MessageReader::readInt64()
    {
        if (_position + sizeof(int64_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        int64_t value;
        std::memcpy(&value, &_data[_position], sizeof(int64_t));
        _position += sizeof(int64_t);
        return value;
    }

    uint64_t MessageReader::readUInt64()
    {
        if (_position + sizeof(uint64_t) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        uint64_t value;
        std::memcpy(&value, &_data[_position], sizeof(uint64_t));
        _position += sizeof(uint64_t);
        return value;
    }

    float MessageReader::readFloat()
    {
        if (_position + sizeof(float) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        float value;
        std::memcpy(&value, &_data[_position], sizeof(float));
        _position += sizeof(float);
        return value;
    }

    double MessageReader::readDouble()
    {
        if (_position + sizeof(double) > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        double value;
        std::memcpy(&value, &_data[_position], sizeof(double));
        _position += sizeof(double);
        return value;
    }

    std::string MessageReader::readString()
    {
        uint16_t length = readUInt16();
        if (_position + length > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        std::string value(reinterpret_cast<const char*>(&_data[_position]), length);
        _position += length;
        return value;
    }

    void MessageReader::readBytes(std::vector<uint8_t>& bytes, size_t count)
    {
        if (_position + count > _data.size())
        {
            throw std::runtime_error("Attempted to read beyond message bounds");
        }
        bytes.assign(_data.begin() + _position, _data.begin() + _position + count);
        _position += count;
    }

    size_t MessageReader::remainingBytes() const
    {
        return _data.size() - _position;
    }

    bool MessageReader::hasMoreData() const
    {
        return _position < _data.size();
    }

    void MessageReader::reset()
    {
        _position = 0;
    }
} 