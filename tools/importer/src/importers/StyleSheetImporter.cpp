/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "StyleSheetImporter.h"
#include <noz/ui/StyleSheet.h>
#include <noz/ui/Style.h>
#include <noz/MetaFile.h>

namespace noz::import
{
    StyleSheetImporter::StyleSheetImporter()
    {
    }
    
    bool StyleSheetImporter::canImport(const std::string& filePath) const
    {
        std::filesystem::path path(filePath);
        return path.extension() == ".styles";
    }
    
    std::vector<std::string> StyleSheetImporter::getSupportedExtensions() const
    {
        return { ".styles" };
    }
    
    std::string StyleSheetImporter::getName() const
    {
        return "StyleSheet Importer";
    }
    
    bool StyleSheetImporter::import(const std::string& sourcePath, const std::string& outputDir)
    {
        std::filesystem::path srcPath(sourcePath);
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / srcPath.stem();
        outputPath.replace_extension(".styles");
        
        std::cout << "Importing stylesheet: " << sourcePath << " -> " << outputPath << std::endl;
        
        return processStyleSheet(sourcePath, outputPath.string());
    }
    
    bool StyleSheetImporter::processStyleSheet(const std::string& sourcePath, const std::string& outputPath)
    {
        // Parse the source file using MetaFile format
        auto meta = noz::MetaFile::parse(sourcePath);
        
        
        auto styleSheet = Object::create<noz::ui::StyleSheet>("styles");
#if 0
        // Create a new stylesheet

        // Load inheritance from meta file if it exists
        std::string metaPath = sourcePath + ".meta";
        if (std::filesystem::exists(metaPath))
        {
            noz::MetaFile styleMetaFile = noz::MetaFile::parse(metaPath);
            
            // Parse inherited stylesheets
            std::string inheritValue = styleMetaFile.getString("StyleSheet", "inherit", ""); // Legacy API for inherit setting
            if (!inheritValue.empty())
            {
                std::istringstream inheritStream(inheritValue);
                std::string inheritedStyleSheet;
                
                while (std::getline(inheritStream, inheritedStyleSheet, ','))
                {
                    // Trim whitespace
                    inheritedStyleSheet.erase(0, inheritedStyleSheet.find_first_not_of(" \t"));
                    inheritedStyleSheet.erase(inheritedStyleSheet.find_last_not_of(" \t") + 1);
                    
                    if (!inheritedStyleSheet.empty())
                    {
                        // Load inherited stylesheet
                        std::string inheritedPath = (std::filesystem::path(sourcePath).parent_path() / (inheritedStyleSheet + ".styles")).string();
                        auto inherited = std::make_shared<noz::ui::StyleSheet>();
                        if (inherited->loadFromFile(inheritedPath))
                        {
                            styleSheet->addInheritedStyleSheet(inherited);
                        }
                        else
                        {
                            std::cerr << "Warning: Failed to load inherited stylesheet: " << inheritedPath << std::endl;
                        }
                    }
                }
            }
        }
#endif
        
        // Parse styles using INI format (each section is a style class)
        std::unordered_map<std::string, noz::ui::Style> styles;
        
        for (const std::string& group : meta.groups())
        {
            // Skip empty group (legacy compatibility)
            if (group.empty())
                continue;
                
            // Create style for this class
            styles[group] = noz::ui::Style::defaultStyle();
            
            // Parse all properties in this group/style
            for (const std::string& propertyName : meta.keys(group))
            {
                parseProperty(group, propertyName, meta, &styles[group]);
            }
        }
        
        // Add all styles to the stylesheet
        for (const auto& [className, style] : styles)
        {
            styleSheet->addStyle(className, style);
        }
        
        // Save the stylesheet
        return styleSheet->saveToFile(outputPath);
    }
    
    bool StyleSheetImporter::parseProperty(const std::string& group, const std::string& propertyName, const noz::MetaFile& metaFile, void* stylePtr)
    {
        auto* style = static_cast<noz::ui::Style*>(stylePtr);
        
        if (propertyName == "width")
        {
            style->width = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "height")
        {
            style->height = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "background-color")
        {
            style->backgroundColor = metaFile.getColor(group, propertyName);
        }
        else if (propertyName == "color")
        {
            style->color = metaFile.getColor(group, propertyName);
        }
        else if (propertyName == "border-color")
        {
            style->borderColor = metaFile.getColor(group, propertyName);
        }
        else if (propertyName == "image-tint")
        {
            style->imageTint = metaFile.getColor(group, propertyName);
        }
        else if (propertyName == "text-outline-color")
        {
            style->textOutlineColor = metaFile.getColor(group, propertyName);
        }
        else if (propertyName == "font-size")
        {
            style->fontSize = metaFile.getFloat(group, propertyName, 16.0f);
        }
        else if (propertyName == "border-radius")
        {
            style->borderRadius = metaFile.getFloat(group, propertyName, 0.0f);
        }
        else if (propertyName == "border-width")
        {
            style->borderWidth = metaFile.getFloat(group, propertyName, 0.0f);
        }
        else if (propertyName == "text-outline-width")
        {
            style->textOutlineWidth = metaFile.getFloat(group, propertyName, 0.0f);
        }
		else if (propertyName == "margin")
		{
			style->marginTop = 
				style->marginLeft =
				style->marginRight = 
				style->marginBottom =
				ui::StyleLength::parse(metaFile.getString(group, propertyName));
		}
        else if (propertyName == "margin-top")
        {
            style->marginTop = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "margin-left")
        {
            style->marginLeft = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "margin-bottom")
        {
            style->marginBottom = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "margin-right")
        {
            style->marginRight = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
		else if (propertyName == "padding")
		{
			style->paddingTop = 
				style->paddingLeft =
				style->paddingRight = 
				style->paddingBottom =
				ui::StyleLength::parse(metaFile.getString(group, propertyName));
		}
        else if (propertyName == "padding-top")
        {
            style->paddingTop = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "padding-left")
        {
            style->paddingLeft = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "padding-bottom")
        {
            style->paddingBottom = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
        else if (propertyName == "padding-right")
        {
            style->paddingRight = ui::StyleLength::parse(metaFile.getString(group, propertyName));
        }
		else if (propertyName == "flex-direction")
		{
			style->flexDirection = ui::StyleEnum<ui::FlexDirection>::parse(metaFile.getString(group, propertyName));
		}
        
        return true;
    }
}