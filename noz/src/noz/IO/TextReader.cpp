///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TextReader.h"

using namespace noz;


TextReader::TextReader(void) {
  peek_ = 0;
}

noz_int32 TextReader::Read(void) {
  char read = 0;
  if(peek_) {
    read = peek_;
    peek_ = 0;
    return read;
  }

  return Read(&read,0,1)==-1?-1:read;
}

noz_int32 TextReader::Peek(void) {
  if(peek_) {
    return (noz_int32)peek_;
  }
  peek_ = 0;
  Read(&peek_,0,1);
  return peek_;
}

String TextReader::ReadLine(void) {
  StringBuilder sb;
  char read = Read();
  while(read != 0) {
    if(read=='\r') {
      if(Peek() == '\n') {
        Read();
      }
      break;
    }
    sb.Append(read);
  }
  return sb.ToString();
}

String TextReader::ReadToEnd(void) {
  StringBuilder sb;
  char read = Read();
  sb.Append(read);
  while(0!=(read=Read())) {
    sb.Append(read);
  }
  return sb.ToString();
}

