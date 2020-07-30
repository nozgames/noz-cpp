///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PrefabImporter_h__
#define __noz_PrefabImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>

namespace noz {

  class PrefabTemplateImporter : public AssetImporter {
    NOZ_OBJECT(ImportEXT=nozprefab,ImportType=PrefabTemplate)

    public: virtual Asset* Import (const String& path) override;
  };

} // namespace noz

#endif // __noz_PrefabImporter_h__

