///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_FactoryFile_h__
#define __noz_Editor_FactoryFile_h__

#include "AssetFile.h"

namespace noz {
namespace Editor {

  class FactoryFile : public AssetFile {
    NOZ_OBJECT(EditorFileExt=nozfactory,EditorAssetType=noz::Factory)

    public: virtual Asset* Import (void) override;
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_FactoryFile_h__

