/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "StyleSheetImporter.h"
#include <noz/ui/StyleSheet.h>
#include <noz/ui/Style.h>
#include <noz/ui/PseudoState.h>
#include <noz/MetaFile.h>
#include <filesystem>
#include <sstream>

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
    
    void StyleSheetImporter::import(const std::string& sourcePath, const std::string& outputDir)
    {
        std::filesystem::path srcPath(sourcePath);
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / srcPath.stem();
        outputPath.replace_extension(".styles");

        auto styleSheet = Object::create<noz::ui::StyleSheet>("styles");
        parseStyles(sourcePath, styleSheet);
        styleSheet->saveToFile(outputPath.string());
    }
    
    std::vector<std::string> StyleSheetImporter::parseInheritList(const std::string& path) const
    {
        std::vector<std::string> inheritedFiles;
        
        auto meta = noz::MetaFile::parse(path);
        auto inherit = meta.getString("StyleSheet", "inherit", "");
        
        if (!inherit.empty())
        {
            std::istringstream inheritStream(inherit);
            std::string inheritedStyleSheet;

            while (std::getline(inheritStream, inheritedStyleSheet, ';'))
            {
                // Trim whitespace
                inheritedStyleSheet.erase(0, inheritedStyleSheet.find_first_not_of(" \t"));
                inheritedStyleSheet.erase(inheritedStyleSheet.find_last_not_of(" \t") + 1);

                if (!inheritedStyleSheet.empty())
                {
                    std::string inheritedPath = (std::filesystem::path(path).parent_path() / (inheritedStyleSheet + ".styles")).string();
                    inheritedFiles.push_back(inheritedPath);
                }
            }
        }
        
        return inheritedFiles;
    }

    void StyleSheetImporter::parseStyles(const std::string& path, std::shared_ptr<noz::ui::StyleSheet> styleSheet)
    {
        auto meta = noz::MetaFile::parse(path);

        // Inherit styles first
        auto inheritedFiles = parseInheritList(path);
        for (const auto& inheritedPath : inheritedFiles)
        {
            parseStyles(inheritedPath, styleSheet);
        }

        std::unordered_map<std::string, noz::ui::Style> styles;
        for (const std::string& group : meta.groups())
        {
            // Skip empty group (legacy compatibility)
            if (group.empty())
                continue;

            // Create style for this class
            styles[group] = noz::ui::Style::default();

            // Parse all properties in this group/style
            for (const std::string& propertyName : meta.keys(group))
            {
                parseProperty(group, propertyName, meta, &styles[group]);
            }
        }

        // Resolve pseudo states by merging base styles with pseudo state styles
        std::unordered_map<std::string, noz::ui::Style> resolvedStyles = styles;
        
        for (const auto& [fullClassName, style] : styles)
        {
            auto [baseName, pseudoState] = noz::ui::parseStyleName(fullClassName);
            
            // If this is a pseudo state style, merge it with the base style
            if (pseudoState != noz::ui::PseudoState::None)
            {
                // Find the base style
                auto baseIt = styles.find(baseName);
                if (baseIt != styles.end())
                {
                    // Create the resolved pseudo style by merging base + pseudo
                    noz::ui::Style resolvedStyle = baseIt->second; // Start with base
                    resolvedStyle.apply(style); // Apply pseudo state overrides
                    resolvedStyles[fullClassName] = resolvedStyle;
                }
            }
        }
        
        // Add all resolved styles to the stylesheet
        for (const auto& [className, style] : resolvedStyles)
            styleSheet->mergeStyle(className, style);
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

    bool StyleSheetImporter::doesDependOn(const std::string& sourcePath, const std::string& dependencyPath)
    {
        std::filesystem::path path(sourcePath);
        if (path.extension() != ".styles")
            return false;

        try
        {
            auto inheritedFiles = parseInheritList(sourcePath);
            
            for (const auto& inheritedPath : inheritedFiles)
            {
                // Check if this file directly depends on the dependency
                bool filesEqual = false;
                try 
                {
                    filesEqual = std::filesystem::equivalent(inheritedPath, dependencyPath);
                }
                catch (const std::exception&)
                {
                    // Fallback to string comparison if filesystem::equivalent fails
                    filesEqual = (inheritedPath == dependencyPath);
                }
                
                if (filesEqual)
                    return true;
                
                // Check recursively if any inherited file depends on the dependency
                if (doesDependOn(inheritedPath, dependencyPath))
                    return true;
            }
        }
        catch (const std::exception&)
        {
            // If we can't parse the file or it doesn't exist, assume no dependency
            return false;
        }

        return false;
    }
}
