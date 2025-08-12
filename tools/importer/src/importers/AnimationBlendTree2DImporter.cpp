#include "AnimationBlendTree2dImporter.h"
#include <filesystem>
#include <iostream>

namespace noz::import
{
    AnimationBlendTree2dImporter::AnimationBlendTree2dImporter()
    {
    }

    bool AnimationBlendTree2dImporter::canImport(const std::string& filePath) const
    {
        return std::filesystem::path(filePath).extension() == ".blendtree2d";
    }

    std::vector<std::string> AnimationBlendTree2dImporter::getSupportedExtensions() const
    {
        return { ".blendtree2d" };
    }

    std::string AnimationBlendTree2dImporter::getName() const
    {
        return "AnimationBlendTree2D Importer";
    }

    void AnimationBlendTree2dImporter::import(const std::string& sourcePath, const std::string& outputDir)
    {
        importBlendTree(sourcePath, outputDir);
    }

    bool AnimationBlendTree2dImporter::importBlendTree(const std::string& sourcePath, const std::string& outputDir)
    {
        try
        {
            std::filesystem::path sourceFile(sourcePath);
            std::filesystem::path outputFile = std::filesystem::path(outputDir) / sourceFile.stem();
            outputFile.replace_extension(".blendtree2d");

            return writeBlendTree(outputFile.string(), sourcePath);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to import blend tree " << sourcePath << ": " << e.what() << std::endl;
            return false;
        }
    }

    bool AnimationBlendTree2dImporter::writeBlendTree(const std::string& outputPath, const std::string& sourcePath)
    {
        try
        {
            // Parse the MetaFile (INI format) from the source
            auto metaFile = noz::MetaFile::parse(sourcePath);
            
            // Extract animation names from the [animations] section
            std::string centerAnim = metaFile.getString("animations", "center", "");
            std::string leftAnim = metaFile.getString("animations", "left", "");
            std::string rightAnim = metaFile.getString("animations", "right", "");
            std::string topAnim = metaFile.getString("animations", "top", "");
            std::string bottomAnim = metaFile.getString("animations", "bottom", "");
            
            // Write binary format
            noz::StreamWriter writer;
            writer.writeFileSignature("BT2D"); // BlendTree2D signature
            writer.writeUInt32(1); // Version
            
            // Write animation references
            writer.writeString(centerAnim);
            writer.writeString(leftAnim);
            writer.writeString(rightAnim);
            writer.writeString(topAnim);
            writer.writeString(bottomAnim);
            
            // Create output directory if needed
            std::filesystem::path outputFile(outputPath);
            std::filesystem::create_directories(outputFile.parent_path());
            
            // Write to file
            if (!writer.writeToFile(outputPath))
            {
                std::cerr << "Failed to write blend tree file: " << outputPath << std::endl;
                return false;
            }
            
            std::cout << "Imported blend tree: " << std::filesystem::path(sourcePath).filename().string() 
                      << " -> " << outputFile.filename().string() << std::endl;
            
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to write blend tree " << outputPath << ": " << e.what() << std::endl;
            return false;
        }
    }
}