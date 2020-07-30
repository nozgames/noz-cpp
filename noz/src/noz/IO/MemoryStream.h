///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_MemoryStream_h__
#define __noz_MemoryStream_h__

namespace noz {

  class MemoryStream : public Stream {
    NOZ_OBJECT()

    private: noz_byte* buffer_;

    private: noz_uint32 position_;

    private: noz_uint32 length_;

    private: noz_uint32 capacity_;

    private: bool external_buffer_;

    /// Initializes a new instance of the MemoryStream class with an expandable capacity initialized to zero.
    public: MemoryStream(void);

    /// Initializes a new instance of the MemoryStream class with an expandable capacity initialized as specified.
    public: MemoryStream(noz_uint32 capacity);

    /// Initializes a non resizable memory stream based on the given byte array
    public: MemoryStream(noz_byte* bytes, noz_uint32 size);

    public: ~MemoryStream (void);

    /// When overridden in a derived class, gets or sets the position within the current stream.
    public: virtual noz_uint32 GetPosition(void) const override {return position_;}

    public: virtual noz_uint32 GetLength(void) const override {return length_;}

    /// Sets the position within the current stream to the specified value.
    public: virtual noz_uint32 Seek(noz_int32 offset, SeekOrigin origin) override;

    /// Reads a block of bytes from the current stream and writes the data to a buffer.
    public: virtual noz_int32 Read(char* buffer, noz_int32 offset, noz_int32 count) override;

    /// Writes a block of bytes to the current stream using data read from a buffer.
    public: virtual noz_int32 Write(char* buffer, noz_int32 offset, noz_int32 count) override;

    /// Returns the array of unsigned bytes from which this stream was created
    public: const noz_byte* GetBuffer(void) const {return buffer_;}

    /// Sets the length of the current stream to the specified value.
    public: void SetLength(noz_int32 length);

    /// Sets the number of bytes allocated for this stream.
    public: void SetCapacity (noz_int32 capacity);

    /// Shrinks the memory stream to fit its contents
    public: void ShrinkToFit (void);
  };

} // namespace noz


#endif //__noz_IO_MemoryStream_h__

