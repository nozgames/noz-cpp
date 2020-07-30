///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TcpClient.h"

using namespace noz;
using namespace noz::Networking;

TcpClient::TcpClient(void) {
  socket_ = nullptr;
  stream_ = nullptr;
  state_ = TcpClientState::NotConnected;
}

TcpClient::TcpClient (Socket* socket) {
  socket_ = socket;
  if(socket_) {
    state_ = TcpClientState::Connected;
    stream_ = new NetworkStream(socket_);
  } else {
    state_ = TcpClientState::NotConnected;
    stream_ = nullptr;
  }
}

TcpClient::~TcpClient(void) {
  delete stream_;
  delete socket_;
}

bool TcpClient::Connect (const IPAddress& addr, noz_int32 port) {  
  // Close any current connection
  Close();

  // Save address and port for async connection
  connect_address_ = addr;
  connect_port_ = port;

  state_ = TcpClientState::Connecting;

  // Create the new socket.
  socket_ = new Socket (SocketType::Seqpacket, ProtocolType::Tcp);

#if 0
  // Start a thread to perform the connection.
  thread_ = new Thread;
  thread_->Start(ThreadStart(this, &TcpClient::AsyncConnect));
#else
  if(!socket_->Connect(addr,port)) {    
    Close();
    return false;
  }
#endif

  return true;
}

void TcpClient::Close(void) {
  delete socket_;
  delete stream_;
  stream_ = nullptr;  
  socket_ = nullptr;
  state_ = TcpClientState::NotConnected;
}

noz_int32 TcpClient::GetAvailable(void) {
  return socket_->GetAvailable();
}

void TcpClient::AsyncConnect(void) {
  if(!socket_->Connect(connect_address_,connect_port_)) {    
    Close();
    return;
  }

  // Create the stream
  stream_ = new NetworkStream(socket_);  
}

void TcpClient::Update (void) {
  switch(state_) {
    case TcpClientState::Connecting:
      if(socket_->Poll(0, SelectMode::Write)) {
        state_ = TcpClientState::Connected;
        stream_ = new NetworkStream(socket_);
      } else if (socket_->Poll(0,SelectMode::Error)) {
        delete socket_;
        socket_ = nullptr;
        state_ = TcpClientState::NotConnected;
      }
      break;

    case TcpClientState::Connected: break;
    case TcpClientState::NotConnected: break;
  }
}

