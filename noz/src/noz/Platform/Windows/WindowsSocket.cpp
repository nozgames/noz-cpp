///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Networking/Dns.h>
#include <noz/Platform/SocketHandle.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

using namespace noz;
using namespace noz::Networking;
using namespace noz::Platform;


namespace noz {
namespace Platform {
namespace Windows {

  class WindowsSocket : public SocketHandle {
    private: SOCKET handle_;
    private: bool is_connecting_;

    public: WindowsSocket(void) {
      // Create the actual socket..
      handle_ = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); 

      unsigned long mode = 1;
      ioctlsocket(handle_, FIONBIO, &mode);
    }

    public: WindowsSocket(SOCKET s) {
      handle_ = s;
    }

    public: virtual bool Connect(const IPAddress& address, noz_int32 port) override {

      SOCKADDR_IN target; //Socket address information

      target.sin_family = AF_INET; // address family Internet
      target.sin_port = htons (port); //Port to connect on
      target.sin_addr.s_addr = address.GetAddressIPv4();


      // Try connecting.
      if (connect(handle_, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR) {
        if(WSAGetLastError() != WSAEWOULDBLOCK) {
          closesocket(handle_);
          return false;
        }
      }

      return true;
    }

    public: virtual bool Bind (const IPAddress& address, noz_int32 port) override {
      SOCKADDR_IN addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons (port);
      addr.sin_addr.s_addr = address.GetAddressIPv4();

      return SOCKET_ERROR != ::bind(handle_,(SOCKADDR*)&addr,sizeof(addr));
    }

    public: virtual bool Listen (noz_int32 backlog) override {
      return SOCKET_ERROR != ::listen(handle_,backlog);
    }

    public: virtual SocketHandle* Accept (void) override {
      SOCKET s = ::accept(handle_,NULL,NULL);
      if(s==INVALID_SOCKET) {
        Console::WriteLine("accept failed with error: %ld\n", WSAGetLastError());
        return nullptr;
      }

      return new WindowsSocket(s);
    }

    public: virtual noz_int32 Send (const noz_byte* bytes, noz_int32 count) override {
      noz_int32 written = 0;
      while(written<count) {
        noz_int32 sent = ::send(handle_,(const char*)(bytes+written), Math::Min(count,4096), 0);
        if(sent == SOCKET_ERROR) return written;
        count -= sent;
        written += sent;
      }
      return written;
    }

    public: virtual noz_int32 Receive(noz_byte* bytes, noz_int32 count) override {
      return (noz_int32)::recv(handle_,(char*)bytes,count,0);
    }

    public: virtual noz_int32 GetAvailable(void) override {
      u_long bytes_available = 0;
      if(ioctlsocket(handle_,FIONREAD,&bytes_available)) return 0;
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
          result = select(1,&fd,nullptr,nullptr,&tv);
          break;

        case SelectMode::Write:
          result = select(1,nullptr,&fd,nullptr,&tv);
          break;

        case SelectMode::Error:
          result = select(1,nullptr,nullptr,&fd,&tv);
          break;
      }

      return result == 1;
    }
  };

}
}
}


IPAddress IPAddress::Parse (const char* address) {
  noz_uint64 addr;
  InetPton(AF_INET,address,&addr);
  return IPAddress((noz_uint32)addr);
}

SocketHandle* SocketHandle::CreateInstance(void) {
  return new Platform::Windows::WindowsSocket;
}

String Dns::GetHostName(void) {
  char buffer[1024];
  if(SOCKET_ERROR==gethostname(buffer,1024)) {
    return "";
  }

  return String(buffer);
}

IPHostEntry Dns::GetHostEntry(const char* hostname) {
  IPHostEntry iphe;

  ADDRINFO hints;
  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  ADDRINFO* ai;
  if(SOCKET_ERROR==GetAddrInfo(hostname,NULL,&hints,&ai)) {
    return iphe;
  }

  iphe.SetHostName(hostname);
  
  for(ADDRINFO* p=ai; p; p=p->ai_next) {
    switch(p->ai_family) {
      case AF_INET:
        iphe.GetAddresses().push_back(IPAddress((noz_uint32)((SOCKADDR_IN*)p->ai_addr)->sin_addr.S_un.S_addr));
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
      SOCKADDR_IN addr;
      memset(&addr,0,sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.S_un.S_addr = GetAddressIPv4();

      char out[128];
      DWORD len = 127;
      WSAAddressToString((SOCKADDR*)&addr,sizeof(addr), NULL, out, &len);
      return String(out);
    }

    default:
      break;
  }

  return String();
}

