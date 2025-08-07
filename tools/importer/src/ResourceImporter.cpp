/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    namespace import
    {
        ResourceImporterRegistry& ResourceImporterRegistry::instance()
        {
            static ResourceImporterRegistry instance;
            return instance;
        }
        
        void ResourceImporterRegistry::registerImporter(std::shared_ptr<ResourceImporter> importer)
        {
            _importers.push_back(importer);
        }
        
        void ResourceImporterRegistry::clear()
        {
            _importers.clear();
        }
        
        std::shared_ptr<ResourceImporter> ResourceImporterRegistry::getImporter(const std::string& filePath) const
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
        
        std::vector<std::shared_ptr<ResourceImporter>> ResourceImporterRegistry::getImporters(const std::string& filePath) const
        {
            std::vector<std::shared_ptr<ResourceImporter>> validImporters;
            for (const auto& importer : _importers)
            {
                if (importer->canImport(filePath))
                {
                    validImporters.push_back(importer);
                }
            }
            return validImporters;
        }
        
        std::vector<std::shared_ptr<ResourceImporter>> ResourceImporterRegistry::getAllImporters() const
        {
            return _importers;
        }
    }
} 