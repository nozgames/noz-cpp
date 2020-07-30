///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_TcpListener_h__
#define __noz_Networking_TcpListener_h__

#include "NetworkStream.h"

namespace noz {
namespace Networking {

  class TcpClient;

  class TcpListener : public Object {
    private: IPAddress addr_;
    private: noz_int32 port_;
    private: Socket* socket_;

    public: TcpListener(const IPAddress& addr, noz_int32 port);

    public: ~TcpListener(void);

    public: bool Start(void);

    public: void Stop(void);

    public: bool IsPending(void);

    public: TcpClient* AcceptTcpClient (void);

    public: Socket* AcceptSocket (void);
  };

} // namespace Networking
} // namespace noz


#endif //__noz_Networking_TcpListener_h__


