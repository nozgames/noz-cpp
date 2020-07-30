///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Networking/IPAddress.h>
#include <noz/Networking/Dns.h>
#include <noz/Platform/SocketHandle.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

using namespace noz;
using namespace noz::Platform;
using namespace noz::Networking;


namespace noz {
namespace Platform {
namespace Posix {

  class PosixSocket : public SocketHandle {
    private: int handle_;

    public: PosixSocket(void) {
      // Create the actual socket..
      handle_ = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

      unsigned long mode = 1;
      ioctl(handle_, FIONBIO, &mode);
    }

    public: PosixSocket(int s) {
      handle_ = s;
    }
    
    public: ~PosixSocket(void) {
      if(handle_!=-1) {
        close(handle_);
      }
    }

    public: virtual bool Connect(const IPAddress& address, noz_int32 port) override {

      sockaddr_in target; //Socket address information

      target.sin_family = AF_INET; // address family Internet
      target.sin_port = htons (port); //Port to connect on
      target.sin_addr.s_addr = address.GetAddressIPv4();


      // Try connecting.
      int err;
      if ((err = connect(handle_, (sockaddr*)&target, sizeof(target))) == -1) {
        err = errno;
        if(err != EINPROGRESS) {
          close(handle_);
          return false;
        }
      }

      return true;
    }

    public: virtual bool Bind (const IPAddress& address, noz_int32 port) override {
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons (port);
      addr.sin_addr.s_addr = address.GetAddressIPv4();

      return -1 != ::bind(handle_,(sockaddr*)&addr,sizeof(addr));
    }

    public: virtual bool Listen (noz_int32 backlog) override {
      return -1 != ::listen(handle_,backlog);
    }

    public: virtual SocketHandle* Accept (void) override {
      int s = ::accept(handle_,NULL,NULL);
      if(s==-1) {
        return nullptr;
      }

      return new PosixSocket(s);
    }

    public: virtual noz_int32 Send (const noz_byte* bytes, noz_int32 count) override {
      return (noz_int32)::send(handle_,(const char*)bytes, count, 0);
    }

    public: virtual noz_int32 Receive(noz_byte* bytes, noz_int32 count) override {
      return (noz_int32)::recv(handle_,(char*)bytes,count,0);
    }

    public: virtual noz_int32 GetAvailable(void) override {
      int bytes_available = 0;
      ioctl(handle_,FIONREAD,&bytes_available);
      return (noz_int32)bytes_available;
    }

    public: virtual bool Poll (noz_int32 wait, SelectMode mode) override {
      fd_set fd;
      struct timeval tv;
      int result = 0;
      tv.tv_usec = wait;
      tv.tv_sec = 0;
      FD_ZERO(&fd);
      FD_SET(handle_, &fd);

      switch(mode) {
        case SelectMode::Read:          
          result = select(handle_+1,&fd,nullptr,nullptr,&tv);
          break;

        case SelectMode::Write:
          result = select(handle_+1,nullptr,&fd,nullptr,&tv);
          break;
          
        case SelectMode::Error:
          result = select(handle_+1,nullptr,nullptr,&fd,&tv);
          break;
          
        default:
          result = 0;
          break;
      }

      return result == 1;
    }
  };

}
}
}


IPAddress IPAddress::Parse (const char* address) {
  return IPAddress((noz_uint32)inet_addr(address));
}

SocketHandle* SocketHandle::CreateInstance(void) {
  return new Platform::Posix::PosixSocket;
}


String Dns::GetHostName(void) {
  char buffer[1024];
  if(0!=gethostname(buffer,1024)) {
    return "";
  }

  return String(buffer);
}


IPHostEntry Dns::GetHostEntry(const char* hostname) {
  IPHostEntry iphe;

  addrinfo hints;
  memset( &hints, 0, sizeof(hints) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* ai;
  if(0==getaddrinfo(hostname,nullptr,&hints,&ai)) {
    return iphe;
  }

  iphe.SetHostName(hostname);
  
  for(addrinfo* p=ai; p; p=p->ai_next) {
    switch(p->ai_family) {
      case AF_INET:
        iphe.GetAddresses().push_back(IPAddress((noz_uint32)((sockaddr_in*)p->ai_addr)->sin_addr.s_addr));
        break;

      case AF_INET6:
        break;
    }
  }

  return iphe;
}


String IPAddress::ToString(void) const {
  switch(family_) {
    case AddressFamily::InterNetwork: {
      sockaddr_in addr;
      memset(&addr,0,sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = GetAddressIPv4();

      char out[128];
      inet_ntop(AF_INET,&addr,out, 127);
      return String(out);
    }

    default:
      break;
  }

  return String();
}
