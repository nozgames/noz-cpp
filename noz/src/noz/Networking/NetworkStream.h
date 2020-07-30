///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_NetworkStream_h__
#define __noz_Networking_NetworkStream_h__

#include <noz/IO/Stream.h>
#include "Socket.h"

namespace noz {
namespace Networking {

  class NetworkStream : public Stream {
    private: Socket* socket_;

    public: NetworkStream(Socket* socket);

    public: virtual noz_int32 Read(char* buffer, noz_int32 offset, noz_int32 count);

    /// Writes a block of bytes to the current stream using data read from a buffer.
    public: virtual noz_int32 Write(char* buffer, noz_int32 offset, noz_int32 count);
  };

} // namespace Networking
} // namespace noz


#endif //__noz_Networking_NetworkStream_h__


