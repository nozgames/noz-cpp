/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    Resources::Resources() {}
    Resources::~Resources() { /* OnUnload will be called by ISingleton::Unload() */ }

    void Resources::unload()
    {
        instance()->unloadAll();
		ISingleton<Resources>::unload();
    }

    void Resources::unloadUnused()
    {
        std::lock_guard<std::mutex> lock(_mutex);    
        // TODO: implement
    }

    void Resources::registerResource(IResource* resource)
    {
        if (!resource)
            return;

        std::lock_guard<std::mutex> lock(_mutex);
        resource->_resourceIndex = _allResources.size();
        _allResources.push_back(resource);
    }

    void Resources::unregisterResource(IResource* resource)
    {
        if (!resource || resource->_resourceIndex == SIZE_MAX)
            return;

        std::lock_guard<std::mutex> lock(_mutex);
        
        // Verify the index is valid
        if (resource->_resourceIndex >= _allResources.size() || _allResources[resource->_resourceIndex] != resource)
        {
            return;
        }

        // Move the last element to this position and pop the last
        size_t lastIndex = _allResources.size() - 1;
        if (resource->_resourceIndex != lastIndex)
        {
            _allResources[resource->_resourceIndex] = _allResources[lastIndex];
            _allResources[resource->_resourceIndex]->_resourceIndex = resource->_resourceIndex;
        }
        
        _allResources.pop_back();
        resource->_resourceIndex = SIZE_MAX;
    }

    void Resources::unloadAll()
    {
		std::lock_guard<std::mutex> lock(_mutex);

        // Unload all tracked resources
        //for (auto* resource : _allResources)
        //{
        //    if (resource)
        //    {
        //        resource->unloadInternal();
        //    }
        //}
        
        _allResources.clear();
    }

    std::string Resources::getFullPath(const std::string& name, const std::string& ext)
    {
        // Get the binary directory using SDL
        const char* basePath = SDL_GetBasePath();
        if (!basePath) {
            // Fallback to current working directory
            std::filesystem::path fullPath = std::filesystem::current_path();
            fullPath /= "resources";
            fullPath /= name + "." + ext;
            return fullPath.string();
        }
        
        std::string binaryDir = basePath;
        
        // Use binary directory + resources
        std::filesystem::path fullPath = binaryDir;
        fullPath /= "resources";
        fullPath /= name + "." + ext;
        return fullPath.string();
    }
}
