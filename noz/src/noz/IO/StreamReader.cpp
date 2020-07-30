///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StreamReader.h"
#include "FileStream.h"

using namespace noz;


StreamReader::StreamReader(Stream* stream) {
  stream_ = stream;
  SkipBOM();
}

StreamReader::~StreamReader(void) {
  stream_ = nullptr;
}

void StreamReader::SkipBOM(void) {
  // Look for Byte Order Mark
  noz_byte BOM[3];
  noz_int32 read = stream_->Read((char*)BOM,0,3);
  if(3!=read||BOM[0]!=0xEF||BOM[1]!=0xBB||BOM[2]!=0xBF) {
    stream_->Seek(-3,SeekOrigin::Current);
  }
}

