/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/tools/Hotload.h>
#include <noz/tools/FileWatcher.h>
#include <noz/ui/StyleSheet.h>
#include <filesystem>
#include <iostream>

namespace noz::tools
{
    // Define static member for extension mappings
    const std::unordered_map<std::string, std::vector<TypeId>> Hotload::_extensionToTypes = {
        { ".styles", { TypeId::of<noz::ui::StyleSheet>() } },
        // Add more mappings as needed
        // { ".texture", { TypeId::of<noz::renderer::Texture>() } },
        // { ".mesh", { TypeId::of<noz::renderer::Mesh>() } },
    };
    
    Hotload::Hotload()
        : _enabled(true)
    {
    }
    
    Hotload::~Hotload()
    {
    }
    
    void Hotload::load()
    {
        ISingleton<Hotload>::load();
        instance()->loadInternal();
    }
    
    void Hotload::unload()
    {
        instance()->unloadInternal();
        ISingleton<Hotload>::unload();
    }
    
    void Hotload::loadInternal()
    {
        if (_fileWatcher)
            return; // Already loaded
            
        _fileWatcher = std::make_unique<noz::tools::FileWatcher>();
        
        // Watch the entire assets directory
		std::string assetsPath = "assets"; // Default assets directory                
        std::cout << "Hotload: Starting to watch assets directory: " << assetsPath << std::endl;
        
        // Watch the assets directory for changes
        bool success = _fileWatcher->watchDirectory(assetsPath, [this](const std::string& changedFile)
        {
            if (_enabled)
            {
                onFileChanged(changedFile);
            }
        });
        
        if (!success)
        {
            std::cerr << "Hotload: Failed to start watching assets directory" << std::endl;
        }
    }
    
    void Hotload::unloadInternal()
    {
        if (_fileWatcher)
        {
            _fileWatcher->stop();
            _fileWatcher.reset();
        }
        
        _reloadCallbacks.clear();
    }
    
    void Hotload::setEnabled(bool enabled)
    {
        _enabled = enabled;
    }
    
    void Hotload::update()
    {
        if (_fileWatcher)
        {
            _fileWatcher->update();
        }
    }
    
    void Hotload::onFileChanged(const std::string& filePath)
    {
        if (!_enabled)
            return;
            
        std::cout << "Hotload: File changed: " << filePath << std::endl;
        
        // Try to reload the file as different resource types
        tryReloadFile(filePath);
    }
    
    void Hotload::tryReloadFile(const std::string& filePath)
    {
        // Get file extension
        std::filesystem::path path(filePath);
        std::string extension = path.extension().string();
        
        // Find which resource types this extension maps to
        auto it = _extensionToTypes.find(extension);
        if (it == _extensionToTypes.end())
        {
            // No registered types for this extension
            return;
        }
        
        // Convert absolute path to resource name
		std::string assetsPath = "assets";
        std::string resourceName;
        
        // Make paths comparable by normalizing them
        std::filesystem::path normalizedFile = std::filesystem::path(filePath).lexically_normal();
        std::filesystem::path normalizedResources = std::filesystem::path(assetsPath).lexically_normal();
        
        // Check if the file is within the assets directory
        auto relative = std::filesystem::relative(normalizedFile, normalizedResources);
        if (!relative.empty() && relative.string().find("..") == std::string::npos)
        {
            // Convert to resource name (remove extension)
            resourceName = relative.replace_extension().generic_string();
        }
        else
        {
            std::cerr << "Hotload: File is not within assets directory: " << filePath << std::endl;
            return;
        }
        
        // Try to reload as each possible type
        for (const auto& typeId : it->second)
        {
            auto callbackIt = _reloadCallbacks.find(typeId);
            if (callbackIt != _reloadCallbacks.end())
            {
                // Special handling for each resource type
                if (typeId == TypeId::of<noz::ui::StyleSheet>())
                {
                    reloadResource<noz::ui::StyleSheet>(resourceName);
                }
                // Add more resource types as needed
            }
        }
    }
}