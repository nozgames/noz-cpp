///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "FileStream.h"
#include "Path.h"
#include "Directory.h"

using namespace noz;


FileStream::FileStream(void) {
  handle_ = nullptr;
}

FileStream::~FileStream(void) {
  delete handle_;
}

bool FileStream::Open (const String& path, FileMode mode, FileAccess access) {
  // Create the file directory if needed
  if(mode!=FileMode::Open) {
    String dir=Path::GetDirectoryName(path);
    if(!dir.IsEmpty()) {
      Directory::CreateDirectory(dir);
    }
  }

  // Create the handle.
  handle_ = FileStreamHandle::CreateInstance(path,mode,access);;
  if(nullptr==handle_) {
    return false;
  }

  filename_ = path;
  return true;  
}

void FileStream::Close(void) {
  if(nullptr==handle_) return;
  delete handle_;
  handle_ = nullptr;
  filename_ = "";
}

noz_int32 FileStream::Read(char* buffer, noz_int32 offset, noz_int32 count) {
  return handle_->Read((buffer+offset),count);
}

noz_int32 FileStream::Write(char* buffer, noz_int32 offset, noz_int32 count) {
  return handle_->Write((buffer+offset),count);
}

noz_uint32 FileStream::Seek(noz_int32 offset, SeekOrigin origin) {
  return handle_->Seek(offset,origin);
}

noz_uint32 FileStream::GetPosition(void) const {
  return handle_->GetPosition();
}

noz_uint32 FileStream::GetLength(void) const {
  return handle_->GetLength();
}

