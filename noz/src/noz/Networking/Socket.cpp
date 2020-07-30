///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Socket.h"
#include <noz/Platform/SocketHandle.h>

using namespace noz;
using namespace noz::Platform;
using namespace noz::Networking;


Socket::Socket(SocketType socket_type, ProtocolType protocol_type) {
  handle_ = nullptr;
}

Socket::~Socket(void) { 
  Close();
}

void Socket::Close(void) {
  delete handle_;
  handle_ = nullptr;
}

bool Socket::Connect(const IPAddress& address, noz_int32 port) {
  handle_ = SocketHandle::CreateInstance();
  if(!handle_->Connect(address,port)) {
    Close();
    return false;
  }

  return true;
}

bool Socket::Bind (const IPAddress& address, noz_int32 port) {
  handle_ = SocketHandle::CreateInstance();
  if(!handle_->Bind(address,port)) {
    Close();
    return false;
  }
  return true;
}

Socket* Socket::Accept (void) {
  Platform::SocketHandle* ah = handle_->Accept();
  if(ah != nullptr) {
    Socket* as = new Socket(socket_type_, protocol_type_);
    as->handle_ = ah;
    return as;
  }

  return nullptr;
}

noz_int32 Socket::Send (const noz_byte* bytes, noz_int32 count) {
  return handle_->Send(bytes,count);
}

noz_int32 Socket::Receive (noz_byte* bytes, noz_int32 count) {
  return handle_->Receive (bytes,count);
}

bool Socket::Poll(noz_int32 wait, SelectMode mode) {
  return handle_->Poll(wait,mode);
}

bool Socket::Listen (noz_int32 backlog) {
  return handle_->Listen(backlog);
}

noz_int32 Socket::GetAvailable(void) {
  return handle_->GetAvailable();
}
