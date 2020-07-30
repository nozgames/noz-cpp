///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_BinaryReader_h__
#define __noz_IO_BinaryReader_h__

namespace noz {

  class BinaryReader : public Object {
    NOZ_OBJECT()

    /// Stream being read from
    private: Stream* stream_;

    /// Construct a new binary reader from the given stream.  Note that the BinaryReader does
    /// not take ownership of the given stream and the pointer should remain valid until
    /// the BinaryReader is deconstructed.
    public: BinaryReader(Stream* stream);
   
    /// Reads a specified maximum number of characters from the current reader and writes the data to a buffer, beginning at the specified index.
    public: noz_int32 Read(char* buffer, noz_int32 offset, noz_int32 count);

    /// Read a boolean value
    public: bool ReadBoolean(void);

    /// Read a single byte
    public: noz_byte ReadByte(void);

    /// Read a 16 bit unsigned integer from the stream
    public: noz_uint16 ReadUInt16 (void);

    /// Read a 32 bit unsigned integer from the stream
    public: noz_uint32 ReadUInt32 (void);

    /// Read a 64 bit unsigned integer from the stream
    public: noz_uint64 ReadUInt64 (void);

    /// Read a 32 bit integer from the stream
    public: noz_int32 ReadInt32 (void);

    /// Read a 64 bit integer from the stream
    public: noz_int64 ReadInt64 (void);

    /// Read a floating point value
    public: noz_float ReadFloat (void);

    /// Read a double value
    public: noz_double ReadDouble (void);

    /// Read a string from the stream
    public: String ReadString (void);

    /// Read a name from the stream
    public: Name ReadName (void);

    /// Read a guid from the stream
    public: Guid ReadGuid (void);
  };

} // namespace noz


#endif //__noz_IO_BinaryReader_h__

