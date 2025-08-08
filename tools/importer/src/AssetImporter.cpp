/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::import
{
    AssetImporterRegistry& AssetImporterRegistry::instance()
    {
        static AssetImporterRegistry instance;
        return instance;
    }
        
    void AssetImporterRegistry::registerImporter(std::shared_ptr<AssetImporter> importer)
    {
        _importers.push_back(importer);
    }
        
    void AssetImporterRegistry::clear()
    {
        _importers.clear();
    }
        
    std::shared_ptr<AssetImporter> AssetImporterRegistry::getImporter(const std::string& filePath) const
    {
        for (const auto& importer : _importers)
        {
            if (importer->canImport(filePath))
            {
                return importer;
            }
        }
        return nullptr;
    }
        
    std::vector<std::shared_ptr<AssetImporter>> AssetImporterRegistry::getImporters(const std::string& filePath) const
    {
        std::vector<std::shared_ptr<AssetImporter>> validImporters;
        for (const auto& importer : _importers)
        {
            if (importer->canImport(filePath))
            {
                validImporters.push_back(importer);
            }
        }
        return validImporters;
    }
        
    std::vector<std::shared_ptr<AssetImporter>> AssetImporterRegistry::getAllImporters() const
    {
        return _importers;
    }
} 