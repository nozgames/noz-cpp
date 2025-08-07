/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    void StreamWriter::ensureCapacity(size_t additionalBytes)
    {
        size_t requiredSize = _position + additionalBytes;
        if (requiredSize > _data.size())
        {
            size_t newSize = std::max(_data.size() * 2, requiredSize);
            _data.resize(newSize);
        }
    }

    void StreamWriter::writeFileSignature(const char* signature, size_t signatureLength)
    {
        write(signature, signatureLength);
    }

    void StreamWriter::clear()
    {
        _data.clear();
        _position = 0;
    }

    bool StreamWriter::writeToFile(const std::string& filePath)
    {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(_data.data()), _position);
        file.close();
        
        return true;
    }

    void StreamWriter::setPosition(size_t position)
    {
        if (position <= _data.size())
        {
            _position = position;
        }
    }

    void StreamWriter::writeUInt8(uint8_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeUInt16(uint16_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeUInt32(uint32_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeUInt64(uint64_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeInt8(int8_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeInt16(int16_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeInt32(int32_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeInt64(int64_t value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeFloat(float value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeDouble(double value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeBool(bool value)
    {
        write(&value, sizeof(value));
    }

    void StreamWriter::writeString(const std::string& value)
    {
        if (value.length() > 65535)
        {
            std::cerr << "StreamWriter: String too long (max 65535 characters)" << std::endl;
            return;
        }
        
        uint16_t length = static_cast<uint16_t>(value.length());
        writeUInt16(length);
        write(value.data(), length);
    }

	void StreamWriter::write(const void* data, size_t size)
	{
		if (size == 0) return;

		ensureCapacity(size);

		if (_position + size > _data.size())
		{
			std::cerr << "StreamWriter: Buffer overflow detected" << std::endl;
			return;
		}

		std::memcpy(_data.data() + _position, data, size);
		_position += size;
	}

	void StreamWriter::writeBytes(const uint8_t* data, size_t size)
	{
		writeUInt32((uint32_t)size);
		write(data, size);
	}

    void StreamWriter::writeBytes(const std::vector<uint8_t>& data)
    {
		writeBytes(data.data(), data.size());
    }
}
