#pragma once

#include "IResource.h"

namespace noz
{
    class Resources : public ISingleton<Resources>
    {
    public:
        Resources();
        ~Resources();

        // Load a resource by path
        template<typename T> std::shared_ptr<T> load(const std::string& path);
                
        template<typename T> void unload(const std::string& path);

        void unloadUnused();

        void unloadAll();

        // Resource tracking methods
        void registerResource(IResource* resource);
        void unregisterResource(IResource* resource);

		static std::string getFullPath(const std::string& name, const std::string& ext);

		static void unload();

    private:

        friend class ISingleton<Resources>;

        mutable std::mutex _mutex;
        std::vector<IResource*> _allResources;

        // Helper method to create resources
        template<typename T> std::shared_ptr<T> loadResource(const std::string& path);
        
        // Thread-safe cache access methods
        template<typename T> std::shared_ptr<T> getCachedResource(const std::string& name);
        template<typename T> void setCachedResource(const std::string& name, const std::shared_ptr<T>& resource);
    };

    // Template implementations
    template<typename T> std::shared_ptr<T> Resources::load(const std::string& name) 
    {
        // Check cache first (thread-safe)
        auto cached = getCachedResource<T>(name);
        if (cached)
            return cached;

        // Load new resource (no mutex lock here)
        auto resource = loadResource<T>(name);
        if (!resource)
            return nullptr;

        // Cache the resource (thread-safe)
        setCachedResource<T>(name, resource);
        return resource;
    }

    template<typename T> void Resources::unload(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = ResourceCache<T>::_resources.find(path);
        if (it != ResourceCache<T>::_resources.end())
        {
            // Call unloadInternal on the resource before removing from cache
            if (it->second)
            {
                it->second->unloadInternal();
            }
            ResourceCache<T>::_resources.erase(it);
        }
    }

    template<typename T> std::shared_ptr<T> Resources::getCachedResource(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return ResourceCache<T>::get(name);
    }

    template<typename T> void Resources::setCachedResource(const std::string& name, const std::shared_ptr<T>& resource)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        ResourceCache<T>::set(name, resource);
    }

    template <typename T> std::shared_ptr<T> Resources::loadResource(const std::string& name)
    {
        // This will be specialized for each resource type
        // For now, return nullptr - specializations will be added later
        return nullptr;
    }

} // namespace noz 
