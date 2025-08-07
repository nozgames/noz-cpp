#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace noz
{
    class StreamReader
    {
    private:
        std::vector<uint8_t> _data;
        size_t _position = 0;

    public:
        StreamReader() = default;
        StreamReader(const std::vector<uint8_t>& data);
        StreamReader(std::vector<uint8_t>&& data);

        // File operations
        bool loadFromFile(const std::string& filePath);
        void setData(const std::vector<uint8_t>& data);
        void setData(std::vector<uint8_t>&& data);
        const std::vector<uint8_t>& getData() const { return _data; }
        void clear();

        // Position management
        size_t position() const { return _position; }
        void setPosition(size_t position);
        size_t getSize() const { return _data.size(); }
        bool isEndOfStream() const { return _position >= _data.size(); }

		size_t seekBegin(size_t offset);

        // Reading methods
        template<typename T>
        T read()
        {
            if (_position + sizeof(T) > _data.size())
            {
                return T{};
            }
			T value{};
			read(&value, sizeof(T));
            return value;
        }

        // File signature validation
        bool readFileSignature(const char* expectedSignature, size_t signatureLength = 4);

        // Specific read methods
		uint8_t readByte() { return readUInt8(); }
        uint8_t readUInt8();
        uint16_t readUInt16();
        uint32_t readUInt32();
        uint64_t readUInt64();
        int8_t readInt8();
        int16_t readInt16();
        int32_t readInt32();
        int64_t readInt64();
        float readFloat();
        double readDouble();
        bool readBool();
        std::string readString();
        std::vector<uint8_t> readBytes(size_t count);
        std::vector<uint8_t> readBytes(); // Reads uint16 size first, then bytes

		template <typename T>
		std::vector<T> readVector()
		{
			std::vector<T> result;
			result.resize(readUInt32());
			read(result.data(), result.size() * sizeof(T));
			return std::move(result);
		}

	private:

		void read(void* dest, size_t size);
    };
} 