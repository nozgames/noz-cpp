/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    AssetDatabase::AssetDatabase() {}
    AssetDatabase::~AssetDatabase() { /* OnUnload will be called by ISingleton::Unload() */ }

    void AssetDatabase::unload()
    {
        instance()->unloadAll();
		ISingleton<AssetDatabase>::unload();
    }

    void AssetDatabase::unloadUnused()
    {
        std::lock_guard<std::mutex> lock(_mutex);    
        // TODO: implement
    }

    void AssetDatabase::registerAsset(Asset* asset)
    {
        if (!asset)
            return;

        std::lock_guard<std::mutex> lock(_mutex);
        asset->_index = _allAssets.size();
        _allAssets.push_back(asset);
    }

    void AssetDatabase::unregisterAsset(Asset* asset)
    {
        if (!asset || asset->_index == SIZE_MAX)
            return;

        std::lock_guard<std::mutex> lock(_mutex);
        
        // Verify the index is valid
        if (asset->_index >= _allAssets.size() || _allAssets[asset->_index] != asset)
        {
            return;
        }

        // Move the last element to this position and pop the last
        size_t lastIndex = _allAssets.size() - 1;
        if (asset->_index != lastIndex)
        {
            _allAssets[asset->_index] = _allAssets[lastIndex];
            _allAssets[asset->_index]->_index = asset->_index;
        }
        
        _allAssets.pop_back();
        asset->_index = SIZE_MAX;
    }

    void AssetDatabase::unloadAll()
    {
		std::lock_guard<std::mutex> lock(_mutex);

        // Unload all tracked assets
        //for (auto* asset : _allAssets)
        //{
        //    if (asset)
        //    {
        //        asset->unloadInternal();
        //    }
        //}
        
        _allAssets.clear();
    }

    std::string AssetDatabase::getFullPath(const std::string& name, const std::string& ext)
    {
        // Get the binary directory using SDL
        const char* basePath = SDL_GetBasePath();
        if (!basePath) {
            // Fallback to current working directory
            std::filesystem::path fullPath = std::filesystem::current_path();
            fullPath /= "assets";
            fullPath /= name + "." + ext;
            return fullPath.string();
        }
        
        std::string binaryDir = basePath;
        
        // Use binary directory + assets
        std::filesystem::path fullPath = binaryDir;
        fullPath /= "assets";
        fullPath /= name + "." + ext;
        return fullPath.string();
    }
}
