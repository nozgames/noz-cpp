#pragma once

#include "../AssetImporter.h"

namespace noz::import
{
    class StyleSheetImporter : public AssetImporter
    {
    public:
        StyleSheetImporter();
            
        bool canImport(const std::string& filePath) const override;
        void import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;            
        bool doesDependOn(const std::string& sourcePath, const std::string& outputDir) override;

    private:
        std::vector<std::string> parseInheritList(const std::string& path) const;
        bool parseProperty(const std::string& group, const std::string& propertyName, const noz::MetaFile& metaFile, void* stylePtr);
        void parseStyles(const std::string& path, std::shared_ptr<noz::ui::StyleSheet> styleSheet);
    };
}
