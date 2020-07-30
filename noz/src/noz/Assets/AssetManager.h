///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AssetManager_h__
#define __noz_AssetManager_h__

#include "AssetArchive.h"
#include "Asset.h"

namespace noz {

  class AssetManager {
    /// Singleton instance of the manager
    private: static AssetManager* instance_;

    /// Map of loaded assets by asset guid.
    private: std::map<Guid,ObjectPtr<Asset>> assets_;

    /// Archives to load files from.  Assets are found by scanning the 
    /// available archives in reverse order.
    private: std::vector<AssetArchive*> archives_;

    /// Private constructor (use Initialize to construct the asset manager)
    private: AssetManager(void);

    /// Private destructor (use Uninitialize to destroy the asset manager)
    private: ~AssetManager(void);

    /// Initialize the static instance of the asset manager
    public: static void Initialize(void);

    /// Uninitialize the static instance of the asset manager.
    public: static void Uninitialize(void);

    /// Add a managed asset 
    public: static void AddAsset (const Guid& guid, Asset* asset);

    /// Add an archive to the asset manager for it to load assets from.  The asset
    /// manager will take ownership of the archive and free it when uninitialized.
    public: static void AddArchive (AssetArchive* archive, bool override=true);

    public: static void RemoveArchive (AssetArchive* archive);

    /// Load an asset of a given type from the asset manager.
    public: template<typename T> static T* LoadAsset(const Name& name);
    public: template<typename T> static T* LoadAsset(const Guid& guid);

    /// Load an asset of a given type from the asset manager.
    public: static Asset* LoadAsset(Type* type, const Name& name);
    public: static Asset* LoadAsset(Type* type, const Guid& guid);

    public: static bool IsAssetLoaded (const Guid& guid);
  };

  template<typename T> T* AssetManager::LoadAsset(const Name& name) {
    return (T*)LoadAsset(typeof(T),name);
  }

  template<typename T> T* AssetManager::LoadAsset(const Guid& guid) {
    return (T*)LoadAsset(typeof(T),guid);
  }

} // namespace noz


#endif // __noz_AssetManager_h__

