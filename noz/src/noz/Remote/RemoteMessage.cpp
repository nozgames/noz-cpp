///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Networking/TcpClient.h>
#include <noz/IO/BinaryWriter.h>
#include "RemoteMessage.h"

using namespace noz;
using namespace noz::Networking;

RemoteMessage::RemoteMessage (RemoteMessageType type, noz_byte* bytes, noz_int32 size) {
  type_ = type;
  stream_ = new MemoryStream(size);
  written_ = 0;

  if(size>0) {
    stream_->SetLength(size);
    memcpy((char*)stream_->GetBuffer(), bytes, size);
  }
}

bool RemoteMessage::Send(TcpClient* client) {
  BinaryWriter writer(client->GetStream());

  // Write message size.
  if(0 == written_) {
    writer.WriteUInt32(stream_->GetLength());

    // Write type.
    writer.WriteByte((noz_byte)type_);
  }

  // Write Body
  if(stream_->GetLength()>0) {
    written_ += writer.Write((char*)(stream_->GetBuffer()+written_), 0, stream_->GetLength()-written_);
  }

  return written_ == stream_->GetLength();
}
