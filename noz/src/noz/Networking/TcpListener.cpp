///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TcpListener.h"
#include "TcpClient.h"

using namespace noz;
using namespace noz::Networking;


TcpListener::TcpListener(const IPAddress& addr, noz_int32 port) : addr_(addr) {
  port_ = port;
  socket_ = nullptr;
}

TcpListener::~TcpListener(void) {
  delete socket_;
}

bool TcpListener::Start(void) {
  socket_ = new Socket (SocketType::Seqpacket, ProtocolType::Tcp);

  if(!socket_->Bind(addr_,port_)) {
    return false;
  }

  socket_->Listen(4);

  return true;
}

void TcpListener::Stop(void) {
  delete socket_;
  socket_= nullptr;
}

bool TcpListener::IsPending(void) {
  return socket_->Poll(0,SelectMode::Read);
}

Socket* TcpListener::AcceptSocket(void) {
  return socket_->Accept();
}

TcpClient* TcpListener::AcceptTcpClient(void) {
  Socket* accept = socket_->Accept();
  if(accept) {
    return new TcpClient(accept);
  }

  return nullptr;  
}
