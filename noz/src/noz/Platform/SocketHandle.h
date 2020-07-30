///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_SocketHandle_h__
#define __noz_Platform_SocketHandle_h__

#include <noz/Networking/Socket.h>

namespace noz {
namespace Platform {

  class SocketHandle {
    public: static SocketHandle* CreateInstance(void);
    
    public: virtual ~SocketHandle(void) {}

    public: virtual bool Connect (const Networking::IPAddress& address, noz_int32 port) = 0;

    public: virtual bool Bind (const Networking::IPAddress& address, noz_int32 port) = 0;

    public: virtual bool Listen (noz_int32 backlog) = 0;

    public: virtual SocketHandle* Accept (void) = 0;

    public: virtual noz_int32 Send (const noz_byte* bytes, noz_int32 count) = 0;
    
    public: virtual noz_int32 Receive (noz_byte* bytes, noz_int32 count) = 0;

    public: virtual noz_int32 GetAvailable(void) = 0;

    public: virtual bool Poll (noz_int32 wait, Networking::SelectMode mode) = 0;
  };        

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_SocketHandle_h__

