///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BinaryReader.h"

using namespace noz;
using namespace noz;


BinaryReader::BinaryReader(Stream* stream) {
  stream_ = stream;
}

noz_int32 BinaryReader::Read(char* buffer, noz_int32 offset, noz_int32 count) {
  return stream_->Read(buffer,offset,count);
}

noz_byte BinaryReader::ReadByte(void) {
  noz_byte v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_uint16 BinaryReader::ReadUInt16(void) {
  noz_uint16 v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_uint32 BinaryReader::ReadUInt32(void) {
  noz_uint32 v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_uint64 BinaryReader::ReadUInt64(void) {
  noz_uint64 v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_int32 BinaryReader::ReadInt32(void) {
  noz_int32 v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_int64 BinaryReader::ReadInt64(void) {
  noz_int64 v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

bool BinaryReader::ReadBoolean(void) {
  bool v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : false;
}

noz_float BinaryReader::ReadFloat (void) {
  noz_float v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

noz_double BinaryReader::ReadDouble (void) {
  noz_double v = 0;
  return sizeof(v) == stream_->Read((char*)&v,0,sizeof(v)) ? v : 0;
}

String BinaryReader::ReadString (void) {
  noz_uint16 size = ReadUInt16();
  if(size==0) {
    return String::Empty;
  }

  return String(stream_,size);
}

Name BinaryReader::ReadName(void) {
  return ReadString();
}

Guid BinaryReader::ReadGuid (void) {
  noz_uint64 ho = 0;
  if(sizeof(ho) != stream_->Read((char*)&ho, 0, sizeof(ho))) return Guid::Empty;
  noz_uint64 lo = 0;
  if(sizeof(lo) != stream_->Read((char*)&lo, 0, sizeof(lo))) return Guid::Empty;
  return Guid(ho,lo);
}

