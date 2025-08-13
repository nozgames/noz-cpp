#pragma once

namespace noz
{
    namespace import
    {
        class AssetImporter
        {
        public:

            virtual ~AssetImporter() = default;
            
            virtual bool canImport(const std::string& filePath) const = 0;
            
            virtual void import(const std::string& sourcePath, const std::string& outputDir) = 0;
            
            virtual bool doesDependOn(const std::string& sourcePath, const std::string& outputDir) { return false; }

            virtual std::vector<std::string> getSupportedExtensions() const = 0;
            
            virtual std::string getName() const = 0;
        };
                
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