///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_Stream_h__
#define __noz_IO_Stream_h__

namespace noz {

  enum class SeekOrigin {
    Begin,
    Current,
    End
  };

  class Stream : public Object {
    NOZ_OBJECT()

    /// When overridden in a derived class, gets or sets the position within the current stream.
    public: virtual noz_uint32 GetPosition(void) const;

    public: virtual noz_uint32 GetLength(void) const {return 0;}

    /// When overridden in a derived class, sets the position within the current stream.
    public: virtual noz_uint32 Seek(noz_int32 offset, SeekOrigin origin);

    public: virtual noz_int32 Read(char* buffer, noz_int32 offset, noz_int32 count) {return -1;}

    /// Writes a block of bytes to the current stream using data read from a buffer.
    public: virtual noz_int32 Write(char* buffer, noz_int32 offset, noz_int32 count) {return -1;}
  };

} // namespace noz


#endif //__noz_IO_Stream_h__

