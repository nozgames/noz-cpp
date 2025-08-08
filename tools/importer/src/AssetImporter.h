#pragma once

namespace noz
{
    namespace import
    {
        class AssetImporter
        {
        public:
            virtual ~AssetImporter() = default;
            
            // Check if this importer can handle the given file
            virtual bool canImport(const std::string& filePath) const = 0;
            
            // Import the asset and save to output directory
            virtual bool import(const std::string& sourcePath, const std::string& outputDir) = 0;
            
            // Get supported file extensions
            virtual std::vector<std::string> getSupportedExtensions() const = 0;
            
            // Get the name of this importer
            virtual std::string getName() const = 0;
        };
                
        // Registry for all resource importers
        class AssetImporterRegistry
        {
        public:
            static AssetImporterRegistry& instance();
            
            void registerImporter(std::shared_ptr<AssetImporter> importer);
            void clear();
            std::shared_ptr<AssetImporter> getImporter(const std::string& filePath) const;
            std::vector<std::shared_ptr<AssetImporter>> getImporters(const std::string& filePath) const;
            std::vector<std::shared_ptr<AssetImporter>> getAllImporters() const;
            
        private:
            std::vector<std::shared_ptr<AssetImporter>> _importers;
        };
    }
} 