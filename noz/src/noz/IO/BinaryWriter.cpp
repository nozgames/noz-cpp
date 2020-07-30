///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BinaryWriter.h"

using namespace noz;


BinaryWriter::BinaryWriter(Stream* stream) {
  stream_ = stream;
}

noz_int32 BinaryWriter::Write(char* buffer, noz_int32 offset, noz_int32 count) {
  return stream_->Write(buffer,offset,count);
}

void BinaryWriter::WriteByte(noz_byte v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteUInt16(noz_uint16 v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteUInt32(noz_uint32 v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteUInt64(noz_uint64 v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteInt32(noz_int32 v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteInt64(noz_int64 v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteBoolean(bool v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteFloat (noz_float v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteDouble (noz_double v) {
  stream_->Write((char*)&v,0,sizeof(v));
}

void BinaryWriter::WriteString (const String& v) {
  WriteUInt16(v.GetLength());
  
  if(v.IsEmpty()) {
    return;
  }

  Write((char*)v.ToCString(), 0, v.GetLength());
}

void BinaryWriter::WriteName(Name v) {
  return WriteString(v.ToCString());
}

void BinaryWriter::WriteGuid (const Guid& guid) {
  noz_uint64 v = guid.GetHighOrder();
  stream_->Write((char*)&v, 0, sizeof(v));
  v = guid.GetLowOrder();
  stream_->Write((char*)&v, 0, sizeof(v));
}
  
