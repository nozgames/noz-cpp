///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DirectoryAssetArchive.h"

using namespace noz;

DirectoryAssetArchive::DirectoryAssetArchive(const String& path) {
  path_ = path;
}

Stream* DirectoryAssetArchive::OpenFile (const Guid& guid) {
  // Attempt to open the file
  return File::CreateStream(GetFileName(guid),FileMode::Open);
}

String DirectoryAssetArchive::GetFileName (const Guid& guid) const {
  return Path::Combine(path_,
    String::Format("%02X/%s.nozasset", (noz_byte)(guid.GetHighOrder()>>56), guid.ToString().ToCString())
   );  
}
