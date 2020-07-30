///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_TcpClient_h__
#define __noz_Networking_TcpClient_h__

#include "NetworkStream.h"

namespace noz {
namespace Networking {

  enum class TcpClientState {
    NotConnected,
    Connecting,
    Connected
  };

  class TcpClient : public Object {
    NOZ_OBJECT()

    private: TcpClientState state_;

    private: Mutex mutex_;

    /// Thread used to keep client async
    private: Thread* thread_;

    /// Socket used for communication to remote
    private: Socket* socket_;

    /// Stream used to read data from socket.
    private: NetworkStream* stream_;

    private: IPAddress connect_address_;

    private: noz_int32 connect_port_;

    /// Create a client without connecting.
    public: TcpClient (void);

    /// Create a client with the given socket.  The client will assume
    /// ownership of the socket and delete it when destroyed
    public: TcpClient (Socket* socket);

    public: ~TcpClient (void);

    public: bool Connect (const IPAddress& addr, noz_int32 port);

    public: void Close(void);

    public: NetworkStream* GetStream(void) {return stream_;}

    public: noz_int32 GetAvailable(void);

    public: bool IsConnected(void) const {return state_ == TcpClientState::Connected; }

    public: bool IsConnecting (void) const {return state_ == TcpClientState::Connecting;}

    public: bool IsDisconnected (void) const {return state_ == TcpClientState::NotConnected;}

    public: void Update (void);

    /// Async connection 
    private: void AsyncConnect (void);
  };

}
} // namespace noz


#endif //__noz_Networking_TcpClient_h__


