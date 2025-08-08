#pragma once

#include "../AssetImporter.h"

namespace noz::import
{
    class StyleSheetImporter : public AssetImporter
    {
    public:
        StyleSheetImporter();
            
        bool canImport(const std::string& filePath) const override;
        bool import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;
            
    private:
        // Parse a .styles source file and create a StyleSheet
        bool processStyleSheet(const std::string& sourcePath, const std::string& outputPath);
            
        // Parse property values into style data using MetaFile helpers
        bool parseProperty(const std::string& group, const std::string& propertyName, const noz::MetaFile& metaFile, void* stylePtr);
    };
}
