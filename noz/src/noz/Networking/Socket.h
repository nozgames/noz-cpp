///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_Socket_h__
#define __noz_Networking_Socket_h__


#include "IPAddress.h"

namespace noz { namespace Platform { class SocketHandle; } }

namespace noz {
namespace Networking {

  enum class SocketType {
    Unknown,
    Dgram,
    Raw,
    Rdm,
    Seqpacket,
    Stream
  };

  enum class ProtocolType {
    Unknown,
    Tcp,
    Udp
  };

  enum class SelectMode {
    Error,
    Read,
    Write
  };

  class Socket : public Object {
    private: SocketType socket_type_;
    private: ProtocolType protocol_type_;
    private: Platform::SocketHandle* handle_;

    public: Socket (SocketType socket_type, ProtocolType protocol_type);

    public: ~Socket (void);

    /// Establish a connection to a remote host.
    public: bool Connect(const IPAddress& address, noz_int32 port);

    public: bool Bind (const IPAddress& address, noz_int32 port);

    public: bool Listen (noz_int32 backlog);

    public: void Close (void);

    public: Socket* Accept (void);

    /// Send the given bytes to the socket.
    public: noz_int32 Send (const noz_byte* bytes, noz_int32 count);

    /// Receive bytes from the socket
    public: noz_int32 Receive (noz_byte* bytes, noz_int32 count);

    public: bool Poll (noz_int32 wait, SelectMode mode);

    public: noz_int32 GetAvailable(void);
  };

} // namespace Networking
} // namespace noz


#endif //__noz_Networking_Socket_h__


