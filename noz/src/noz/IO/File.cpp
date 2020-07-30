///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "File.h"

using namespace noz;


FileStream* File::CreateStream (const String& path, FileMode mode, FileAccess access) {
  FileStream* fs = new FileStream;
  if(!fs->Open(path,mode,access)) {
    delete fs;
    return nullptr;
  }
  return fs;
}
