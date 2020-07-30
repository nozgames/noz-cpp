///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Directory.h"
#include "Path.h"

using namespace noz;


bool Directory::CreateDirectory(String path) {
  if(!CreateDirectoryInternal(path)) {
    // TODO: throw exception
    return false;
  }

  return true;
}

bool Directory::Copy (const String& from, const String& to, bool overwrite) {
  if(!CreateDirectory(to)) return false;

  std::vector<String> files = Directory::GetFiles(from, true);
  for(noz_uint32 i=0,c=files.size(); i<c; i++) {
    String dst = Path::Canonical(Path::Combine(to,Path::GetRelativePath(files[i],from)));
    if(!CreateDirectory(Path::GetDirectoryName(dst))) return false;
    if(!File::Copy(files[i], dst, overwrite)) return false;
  }

  return true;
}

