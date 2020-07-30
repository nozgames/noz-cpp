///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "NetworkStream.h"

using namespace noz;
using namespace noz::Networking;

NetworkStream::NetworkStream(Socket* socket) {
  socket_ = socket;
}

noz_int32 NetworkStream::Read(char* buffer, noz_int32 offset, noz_int32 count) {
  return (noz_int32)socket_->Receive((noz_byte*)(buffer+offset), count);
}

noz_int32 NetworkStream::Write(char* buffer, noz_int32 offset, noz_int32 count) {
  return (noz_int32)socket_->Send((const noz_byte*)(buffer+offset), count);
}

