///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_PrefabImporter_h__
#define __noz_Editor_PrefabImporter_h__

#include "AssetFile.h"

namespace noz {
namespace Editor {

  class PrefabFile : public AssetFile {
    NOZ_OBJECT(EditorFileExt=nozprefab,EditorAssetType=noz::Prefab)

    public: virtual bool CreateEmpty (void) override;

    public: virtual Asset* Import (void) override;

    public: virtual bool Reimport (Asset* asset) override;

    public: virtual bool CanCreateNode (void) const override {return true;}

    public: virtual Node* CreateNode (void) const override;

    private: bool Import (Prefab* prefab);
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_PrefabImporter_h__
