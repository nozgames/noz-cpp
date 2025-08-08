/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    StreamReader::StreamReader(const std::vector<uint8_t>& data)
        : _data(data), _position(0)
    {
    }

    StreamReader::StreamReader(std::vector<uint8_t>&& data)
        : _data(std::move(data)), _position(0)
    {
    }

    bool StreamReader::loadFromFile(const std::string& filePath)
    {
        if (!std::filesystem::exists(filePath))
            return false;

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
            return false;

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        _data.resize(fileSize);
        file.read(reinterpret_cast<char*>(_data.data()), fileSize);
        _position = 0;

        return true;
    }

    void StreamReader::setData(const std::vector<uint8_t>& data)
    {
        _data = data;
        _position = 0;
    }

    void StreamReader::setData(std::vector<uint8_t>&& data)
    {
        _data = std::move(data);
        _position = 0;
    }

    void StreamReader::clear()
    {
        _data.clear();
        _position = 0;
    }

    void StreamReader::setPosition(size_t position)
    {
        if (position <= _data.size())
        {
            _position = position;
        }
    }

    bool StreamReader::readFileSignature(const char* expectedSignature, size_t signatureLength)
    {
        if (_position + signatureLength > _data.size())
        {
            return false;
        }

        if (std::memcmp(_data.data() + _position, expectedSignature, signatureLength) != 0)
        {
            return false;
        }

        _position += signatureLength;
        return true;
    }

    uint8_t StreamReader::readUInt8()
    {
        return read<uint8_t>();
    }

    uint16_t StreamReader::readUInt16()
    {
        return read<uint16_t>();
    }

    uint32_t StreamReader::readUInt32()
    {
        return read<uint32_t>();
    }

    uint64_t StreamReader::readUInt64()
    {
        return read<uint64_t>();
    }

    int8_t StreamReader::readInt8()
    {
        return read<int8_t>();
    }

    int16_t StreamReader::readInt16()
    {
        return read<int16_t>();
    }

    int32_t StreamReader::readInt32()
    {
        return read<int32_t>();
    }

    int64_t StreamReader::readInt64()
    {
        return read<int64_t>();
    }

    float StreamReader::readFloat()
    {
        return read<float>();
    }

    double StreamReader::readDouble()
    {
        return read<double>();
    }

    bool StreamReader::readBool()
    {
        return read<bool>();
    }

    std::string StreamReader::readString()
    {
        uint16_t length = readUInt16();
        if (_position + length > _data.size())
        {
            return "";
        }

        std::string result(reinterpret_cast<const char*>(_data.data() + _position), length);
        _position += length;
        return result;
    }

    std::vector<uint8_t> StreamReader::readBytes(size_t count)
    {
        if (_position + count > _data.size())
        {
            return {};
        }

        std::vector<uint8_t> result(_data.begin() + _position, _data.begin() + _position + count);
        _position += count;
        return result;
    }

    std::vector<uint8_t> StreamReader::readBytes()
    {
        size_t size = readUInt32();
        return readBytes(size);
    }

	size_t StreamReader::seekBegin(size_t position)
	{
		if (position <= _data.size())
			_position = position;

		return _position;
	}

	void StreamReader::read(void* dest, size_t size)
	{
		if (_position + size > _data.size())
			return;

		memcpy(((uint8_t*)dest), _data.data() + _position, size);
		_position += size;
	}
} 
