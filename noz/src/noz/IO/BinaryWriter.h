///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_BinaryWriter_h__
#define __noz_IO_BinaryWriter_h__

namespace noz {

  class BinaryWriter : public Object {
    NOZ_OBJECT(NoAllocator)

    /// Stream being Write from
    private: Stream* stream_;

    /// Construct a new binary Writeer to the given stream.
    public: BinaryWriter(Stream* stream);
   
    /// Writes a specified maximum number of characters to the current Writeer and writes the data to a buffer, beginning at the specified index.
    public: noz_int32 Write(char* buffer, noz_int32 offset, noz_int32 count);

    /// Write a boolean value
    public: void WriteBoolean(bool v);

    /// Write a single byte
    public: void WriteByte(noz_byte v);

    /// Write a 16 bit unsigned integer to the stream
    public: void WriteUInt16 (noz_uint16 v);

    /// Write a 32 bit unsigned integer to the stream
    public: void WriteUInt32 (noz_uint32 v);

    /// Write a 64 bit unsigned integer to the stream
    public: void WriteUInt64 (noz_uint64 v);

    /// Write a 32 bit integer to the stream
    public: void WriteInt32 (noz_int32 v);

    /// Write a 64 bit integer to the stream
    public: void WriteInt64 (noz_int64 v);

    /// Write a floating point value
    public: void WriteFloat (noz_float v);

    /// Write a double value
    public: void WriteDouble (noz_double v);

    /// Write a string to the stream
    public: void WriteString (const String& v);

    /// Write a name to the stream
    public: void WriteName (Name v);

    /// Write a guid
    public: void WriteGuid (const Guid& guid);
  };

} // namespace noz


#endif //__noz_IO_BinaryWriter_h__

