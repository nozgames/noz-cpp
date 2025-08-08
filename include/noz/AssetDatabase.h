#pragma once

#include "Asset.h"

namespace noz
{
    class AssetDatabase : public ISingleton<AssetDatabase>
    {
    public:
        ~AssetDatabase();

        // Load a asset by path
        template<typename T> std::shared_ptr<T> load(const std::string& path);
                
        template<typename T> void unload(const std::string& path);

        void unloadUnused();

        void unloadAll();

        // Asset tracking methods
        void registerAsset(Asset* asset);
        void unregisterAsset(Asset* asset);

		static std::string getFullPath(const std::string& name, const std::string& ext);

		static void unload();

    private:

        friend class ISingleton<AssetDatabase>;

        AssetDatabase();

        mutable std::mutex _mutex;
        std::vector<Asset*> _allAssets;

        // Helper method to create resources
        template<typename T> std::shared_ptr<T> loadInternal(const std::string& path);
        
        // Thread-safe cache access methods
        template<typename T> std::shared_ptr<T> getCachedAsset(const std::string& name);
        template<typename T> void setCachedAsset(const std::string& name, const std::shared_ptr<T>& asset);
    };

    // Template implementations
    template<typename T> std::shared_ptr<T> AssetDatabase::load(const std::string& name) 
    {
        // Check cache first (thread-safe)
        auto cached = getCachedAsset<T>(name);
        if (cached)
            return cached;

        // Load new asset (no mutex lock here)
        auto asset = loadInternal<T>(name);
        if (!asset)
            return nullptr;

        // Cache the asset (thread-safe)
        setCachedAsset<T>(name, asset);
        return asset;
    }

    template<typename T> void AssetDatabase::unload(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = AssetCache<T>::_resources.find(path);
        if (it != AssetCache<T>::_resources.end())
        {
            // Call unloadInternal on the asset before removing from cache
            if (it->second)
            {
                it->second->unloadInternal();
            }
            AssetCache<T>::_resources.erase(it);
        }
    }

    template<typename T> std::shared_ptr<T> AssetDatabase::getCachedAsset(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return AssetCache<T>::get(name);
    }

    template<typename T> void AssetDatabase::setCachedAsset(const std::string& name, const std::shared_ptr<T>& asset)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        AssetCache<T>::set(name, asset);
    }

    template <typename T> std::shared_ptr<T> AssetDatabase::loadInternal(const std::string& name)
    {
        // This will be specialized for each asset type
        // For now, return nullptr - specializations will be added later
        return nullptr;
    }

    template<typename TAsset> static std::shared_ptr<TAsset> Asset::load(const std::string& name)
    {
		return AssetDatabase::instance()->load<TAsset>(name);
    }
}
