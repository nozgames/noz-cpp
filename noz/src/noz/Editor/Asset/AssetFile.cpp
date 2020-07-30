///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetFile.h"

using namespace noz;
using namespace noz::Editor;

AssetFile::AssetFile(void) {
}

AssetEditor* AssetFile::CreateEditor (void) const {
  return AssetDatabase::CreateAssetEditor((AssetFile*)this);
}

DateTime AssetFile::GetModifiedDate(void) const {
  return File::GetLastWriteTime(path_);
}


