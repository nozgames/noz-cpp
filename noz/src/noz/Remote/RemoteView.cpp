///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Networking/IPAddress.h>
#include <noz/Networking/Dns.h>
#include <noz/Networking/TcpClient.h>
#include <noz/IO/BinaryReader.h>
#include "RemoteView.h"

using namespace noz;
using namespace noz::Networking;

/// Time in seconds for the remote client to timeout and reset connection.
static const noz_float RemoteClientTimeout = 1.0f;

/// Check for new connetions every second.
static const noz_float RemoteClientConnectionRetry = 1.0f;

/// Port to connect client to
static const noz_int32 RemoteClientServerPort = 7878;


RemoteView::RemoteView(void) {
  receive_size_ = 0;
  receive_time_ = Time::GetApplicationTime();
  client_ = nullptr;
}

RemoteView::~RemoteView(void) {
}

void RemoteView::ProcessMessageQueue (void) {
  if(client_ == nullptr) return;

  while(!send_queue_.empty()) {    
    if (!send_queue_[0]->Send(client_)) return;
    send_queue_.erase(send_queue_.begin());
  }
}


RemoteMessage* RemoteView::GetMessage (void) {
  // If there is no client set on the dispatcher then there are no messages..
  if(nullptr == client_) return nullptr;

  // Available bytes..
  noz_int32 avail = client_->GetAvailable();

  // If no available bytes then there are no new messages
  if(avail == 0) return nullptr;

  // Track the last time a byte was received
  receive_time_ = Time::GetApplicationTime();

  // New message?
  if(receive_size_==0) {
    // Wait until there are enough bytes for the entire message header
    if(avail < 5) return nullptr;

    // Read the message header.
    BinaryReader reader(client_->GetStream());
    receive_size_ = reader.ReadUInt32();
    receive_type_ = (RemoteMessageType)reader.ReadByte();

    // Subtract the header from the available size.
    avail -= (sizeof(noz_uint32)+sizeof(noz_byte));

    // Zero length message?
    if(receive_size_==0) return new RemoteMessage(receive_type_, nullptr, 0);

    // Initialize the receive stream to prepare for the message
    receive_stream_.SetLength(0);
    receive_stream_.SetCapacity(receive_size_);
    memset((char*)receive_stream_.GetBuffer(),0,receive_size_);
  }
 
  // Message continued?
  if(receive_size_ <= 0) return nullptr;

  // Cap the size at how much is needed for this message
  noz_int32 size = Math::Min(avail, (noz_int32)(receive_size_ - receive_stream_.GetLength()));

  // Nothing to read?
  if(size <=0) return nullptr;

  // Read in as much as we can to the message
  noz_int32 read = client_->GetStream()->Read((char*)receive_stream_.GetBuffer(), receive_stream_.GetLength(), size);
  receive_stream_.SetLength(receive_stream_.GetLength()+read);
  avail -= read;
        
  // Done?
  if(receive_stream_.GetLength()==receive_size_) {
    RemoteMessage* msg = new RemoteMessage(receive_type_, (noz_byte*)receive_stream_.GetBuffer(), receive_size_);
    receive_size_ = 0;
    return msg;
  }

  // No message
  return nullptr;
}

void RemoteView::PutMessage(RemoteMessage* msg) {
  if(msg) send_queue_.push_back(msg);
}
