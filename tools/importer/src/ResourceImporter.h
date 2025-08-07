#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ctime>

namespace noz
{
    namespace import
    {
        class ResourceImporter
        {
        public:
            virtual ~ResourceImporter() = default;
            
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
        class ResourceImporterRegistry
        {
        public:
            static ResourceImporterRegistry& instance();
            
            void registerImporter(std::shared_ptr<ResourceImporter> importer);
            void clear();
            std::shared_ptr<ResourceImporter> getImporter(const std::string& filePath) const;
            std::vector<std::shared_ptr<ResourceImporter>> getImporters(const std::string& filePath) const;
            std::vector<std::shared_ptr<ResourceImporter>> getAllImporters() const;
            
        private:
            std::vector<std::shared_ptr<ResourceImporter>> _importers;
        };
    }
} 