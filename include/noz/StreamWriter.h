#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace noz
{
    class StreamWriter
    {
    private:
        std::vector<uint8_t> _data;
        size_t _position = 0;

        void ensureCapacity(size_t additionalBytes);

    public:
        StreamWriter() = default;

        // File signature writing
        void writeFileSignature(const char* signature, size_t signatureLength = 4);

        // Data access
        const std::vector<uint8_t>& getData() const { return _data; }
        std::vector<uint8_t> moveData() { return std::move(_data); }
        void clear();

        // File operations
        bool writeToFile(const std::string& filePath);

        // Position management
        size_t getPosition() const { return _position; }
        void setPosition(size_t position);
        size_t getSize() const { return _data.size(); }

        // Template write method
        template<typename T>
        void write(const T& value)
        {
            ensureCapacity(sizeof(T));
            *reinterpret_cast<T*>(_data.data() + _position) = value;
            _position += sizeof(T);
        }

        // Specific write methods
        void writeUInt8(uint8_t value);
        void writeUInt16(uint16_t value);
        void writeUInt32(uint32_t value);
        void writeUInt64(uint64_t value);
        void writeInt8(int8_t value);
        void writeInt16(int16_t value);
        void writeInt32(int32_t value);
        void writeInt64(int64_t value);
        void writeFloat(float value);
        void writeDouble(double value);
        void writeBool(bool value);
        void writeString(const std::string& value);

		template <typename T> void writeVector(const std::vector<T>& vec) 		
		{
			writeUInt32(static_cast<uint32_t>(vec.size()));
			write(vec.data(), vec.size() * sizeof(T));	
		}

		void writeBytes(const std::vector<uint8_t>& data);
		void writeBytes(const uint8_t* data, size_t size);

		void write(const void* data, size_t size);
	};
}
