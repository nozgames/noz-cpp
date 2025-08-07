/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ISingleton.h>
#include <noz/Resources.h>
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
        // Callback when a resource is reloaded
        template<typename T>
        using ReloadCallback = std::function<void(const std::string& resourceName, std::shared_ptr<T> resource)>;
        
        Hotload();
        ~Hotload();
        
        // Static load/unload methods following singleton pattern
        static void load();
        static void unload();
        
        // Enable/disable hot reloading
        void setEnabled(bool enabled);
        bool isEnabled() const { return _enabled; }
        
        // Register a resource type for hot reloading with a callback
        template<typename T>
        void registerType(ReloadCallback<T> callback)
        {
            TypeId typeId = TypeId::of<T>();
            _reloadCallbacks[typeId] = [callback](const std::string& name, void* resource) 
            {
                callback(name, *static_cast<std::shared_ptr<T>*>(resource));
            };
        }
        
        // Unregister a resource type
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
        
        // Reload a specific resource
        template<typename T>
        void reloadResource(const std::string& resourceName)
        {
            // Get the existing resource from cache
            auto resource = ResourceCache<T>::get(resourceName);
            if (resource)
            {
                // Call the resource's reload method
                resource->reload();
                
                // Call the registered callback
                TypeId typeId = TypeId::of<T>();
                auto it = _reloadCallbacks.find(typeId);
                if (it != _reloadCallbacks.end())
                {
                    it->second(resourceName, &resource);
                }
            }
            else
            {
                std::cerr << "Hotload: Resource not found in cache: " << resourceName << std::endl;
            }
        }
        
        // Try to reload a file as different resource types
        void tryReloadFile(const std::string& filePath);
        
        bool _enabled = true;
        std::unique_ptr<noz::tools::FileWatcher> _fileWatcher;
        std::unordered_map<TypeId, std::function<void(const std::string&, void*)>> _reloadCallbacks;
        
        // Track which extensions map to which resource types
        static const std::unordered_map<std::string, std::vector<TypeId>> _extensionToTypes;
    };
}