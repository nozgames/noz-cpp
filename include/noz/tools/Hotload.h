/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ISingleton.h>
#include <noz/TypeId.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace noz::tools
{
	class FileWatcher;

    class Hotload : public ISingleton<Hotload>
    {
    public:
        // Callback when a asset is reloaded
        template<typename T>
        using ReloadCallback = std::function<void(const std::string& assetName, std::shared_ptr<T> asset)>;
        
        Hotload();
        ~Hotload();
        
        // Static load/unload methods following singleton pattern
        static void load();
        static void unload();
        
        // Enable/disable hot reloading
        void setEnabled(bool enabled);
        bool isEnabled() const { return _enabled; }
        
        // Register a asset type for hot reloading with a callback
        template<typename T>
        void registerType(ReloadCallback<T> callback)
        {
            TypeId typeId = TypeId::of<T>();
            _reloadCallbacks[typeId] = [callback](const std::string& name, void* asset) 
            {
                callback(name, *static_cast<std::shared_ptr<T>*>(asset));
            };
        }
        
        // Unregister a asset type
        template<typename T>
        void unregisterType()
        {
            TypeId typeId = TypeId::of<T>();
            _reloadCallbacks.erase(typeId);
        }
        
        // Process queued callbacks on main thread
        void update();
        
    private:
        // Friend declaration for ISingleton access
        friend class ISingleton<Hotload>;
        
        // Internal load/unload methods
        void loadInternal();
        void unloadInternal();
        
        // File changed callback
        void onFileChanged(const std::string& filePath);
        
        // Reload a specific asset
        template<typename T>
        void reloadResource(const std::string& assetName)
        {
            // Get the existing asset from cache
            auto asset = AssetCache<T>::get(assetName);
            if (asset)
            {
                // Call the asset's reload method
                asset->reload();
                
                // Call the registered callback
                TypeId typeId = TypeId::of<T>();
                auto it = _reloadCallbacks.find(typeId);
                if (it != _reloadCallbacks.end())
                {
                    it->second(assetName, &asset);
                }
            }
            else
            {
                std::cerr << "Hotload: Resource not found in cache: " << assetName << std::endl;
            }
        }
        
        // Try to reload a file as different asset types
        void tryReloadFile(const std::string& filePath);
        
        bool _enabled = true;
        std::unique_ptr<noz::tools::FileWatcher> _fileWatcher;
        std::unordered_map<TypeId, std::function<void(const std::string&, void*)>> _reloadCallbacks;
        
        // Track which extensions map to which asset types
        static const std::unordered_map<std::string, std::vector<TypeId>> _extensionToTypes;
    };
}