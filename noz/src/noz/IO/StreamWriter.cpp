///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StreamWriter.h"
#include "FileStream.h"

using namespace noz;


StreamWriter::StreamWriter(Stream* stream) {
  stream_ = stream;
}

StreamWriter::~StreamWriter(void) {
  stream_ = nullptr;
}

void StreamWriter::WriteInternal (const char* buffer, noz_int32 index, noz_int32 count) {
  stream_->Write((char*)buffer,index,count);
}
