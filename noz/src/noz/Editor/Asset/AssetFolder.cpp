///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetFolder.h"

using namespace noz;
using namespace noz::Editor;

AssetFolder::AssetFolder(void) {
}

bool AssetFolder::Contains (AssetFile* file, bool recursive) const {
  if(file->GetFolder() == this) return true;
  if(recursive) {
    for(AssetFolder* folder=file->GetFolder()->parent_; folder; folder=folder->parent_) {
      if(folder==this) return true;
    }
  }
  return false;
}