/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "ResourceImporter.h"
#include "importers/FontImporter.h"
#include "importers/TextureImporter.h"
#include "importers/MeshImporter.h"
#include "importers/SkeletonImporter.h"
#include "importers/AnimationImporter.h"
#include "importers/ShaderImporter.h"
#include "importers/StyleSheetImporter.h"
#include "importers/AnimationBlendTree2DImporter.h"
#include <unordered_set>

namespace noz::import
{
    static void registerImporters(const ImportConfig& config)
    {
        auto& registry = ResourceImporterRegistry::instance();
        
        // Clear existing importers to avoid duplicates
        registry.clear();            
        registry.registerImporter(std::make_shared<FontImporter>(config.font));
        registry.registerImporter(std::make_shared<TextureImporter>(config.texture));
        registry.registerImporter(std::make_shared<MeshImporter>(config.model));
        registry.registerImporter(std::make_shared<SkeletonImporter>(config.model));
        registry.registerImporter(std::make_shared<AnimationImporter>(config.model));
        registry.registerImporter(std::make_shared<ShaderImporter>(config.shader));
        registry.registerImporter(std::make_shared<StyleSheetImporter>());
		registry.registerImporter(std::make_shared<AnimationBlendTree2dImporter>());
    }

    bool importFile(const std::string& filePath, const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config)
    {
        // Skip .meta files
        if (filePath.find(".meta") != std::string::npos)
            return true;

        // Make sure file exists
        if (!std::filesystem::exists(filePath))
        {
            std::cerr << "File does not exist: " << filePath << std::endl;
            return false;
        }

        // Register importers if needed
        auto& registry = ResourceImporterRegistry::instance();
        if (registry.getImporters(filePath).empty())
        {
            registerImporters(config);
        }

        // Get importers for this file
        auto importers = registry.getImporters(filePath);
        if (importers.empty())
        {
            // No importer for this file type
            std::cout << "No importer found for: " << filePath << std::endl;
            return true;
        }

        // Calculate relative path and output directory
        std::filesystem::path sourcePath(filePath);
        std::filesystem::path relativePath = std::filesystem::relative(sourcePath, sourceDir);
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / relativePath.parent_path();

        // Create output directory if needed
        std::filesystem::create_directories(outputPath);

        // Import with all applicable importers
        bool anySucceeded = false;
        for (const auto& importer : importers)
        {
            if (importer->import(filePath, outputPath.string()))
            {
				std::cout << "imported '" + filePath + "'" << std::endl;
				anySucceeded = true;
            }
            else
            {
                std::cerr << "Failed to import " << relativePath.string() << " with " << importer->getName() << std::endl;
            }
        }

        return anySucceeded;
    }

    bool import(const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config)
    {
        std::cout << "Starting import process..." << std::endl;
        std::cout << "Source directory: " << sourceDir << std::endl;
        std::cout << "Output directory: " << outputDir << std::endl;
            
        if (!std::filesystem::exists(sourceDir))
        {
            std::cerr << "Source directory does not exist: " << sourceDir << std::endl;
            return false;
        }
            
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outputDir);
        std::cout << "Created output directory: " << outputDir << std::endl;
            
        // Register all importers
        registerImporters(config);
        auto& registry = ResourceImporterRegistry::instance();
                        
        std::vector<std::future<bool>> futures;
        int processedFiles = 0;
        int failedFiles = 0;
            
        // Recursively scan source directory
        std::cout << "Scanning source directory for files..." << std::endl;
        int totalFiles = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
        {
			if (!entry.is_regular_file())
				continue;

			std::string filePath = entry.path().string();
				
			// Skip .meta files - they are only used during import, not as assets
			std::filesystem::path path(filePath);
			if (path.extension() == ".meta")
				continue;
				
			auto importers = registry.getImporters(filePath);
			if (importers.empty())
				continue;

            totalFiles++;
                    
            // Calculate relative path from source directory
            std::filesystem::path sourcePath(filePath);
            std::filesystem::path relativePath = std::filesystem::relative(sourcePath, sourceDir);
            std::filesystem::path outputPath = std::filesystem::path(outputDir) / relativePath.parent_path();
                        
            // Create output directory if it doesn't exist
            std::filesystem::create_directories(outputPath);
                
            // Process file with all applicable importers
            bool anySucceeded = false;
            for (const auto& importer : importers)
            {
                if (importer->import(filePath, outputPath.string()))
                {
					std::cout << "imported '" + filePath + "'" << std::endl;
                    anySucceeded = true;
                }
                else
                {
                    std::cerr << "Failed to import " << filePath << " with " << importer->getName() << std::endl;
                }
            }
                
            if (anySucceeded)
            {
                processedFiles++;
            }
            else
            {
                std::cerr << "All importers failed for: " << filePath << std::endl;
                failedFiles++;
            }
        }
            
        if (totalFiles == 0)
            return true;
            

        // Consider it a success if we processed at least some files and failures are minimal
        return (processedFiles > 0) && (failedFiles <= processedFiles / 2);
    }

    bool deleteOutputFile(const std::string& deletedFilePath, const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config)
    {
        // Register importers to determine what output files would have been created
        registerImporters(config);
        auto& registry = ResourceImporterRegistry::instance();
        
        auto importers = registry.getImporters(deletedFilePath);
        if (importers.empty())
        {
            std::cout << "No importers found for deleted file: " << deletedFilePath << std::endl;
            return true; // Nothing to delete
        }

        // Calculate relative path and output directory
        std::filesystem::path sourcePath(deletedFilePath);
        std::filesystem::path relativePath = std::filesystem::relative(sourcePath, sourceDir);
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / relativePath.parent_path();

        bool anyDeleted = false;
        
        // For each importer, determine what output files it would have created
        for (const auto& importer : importers)
        {
            // Most importers create a file with the same name in the output directory
            std::filesystem::path outputFilePath = outputPath / sourcePath.filename();
            
            // Check for different output extensions based on importer type
            std::string importerName = importer->getName();
            if (importerName == "MeshImporter")
            {
                // MeshImporter might create .mesh files
                outputFilePath.replace_extension(".mesh");
            }
            else if (importerName == "SkeletonImporter")
            {
                // SkeletonImporter might create .skeleton files
                outputFilePath.replace_extension(".skeleton");
            }
            else if (importerName == "AnimationImporter")
            {
                // AnimationImporter might create .animation files
                outputFilePath.replace_extension(".animation");
            }
            else if (importerName == "FontImporter")
            {
                // FontImporter might create .font files
                outputFilePath.replace_extension(".font");
            }
            else if (importerName == "ShaderImporter")
            {
                // ShaderImporter might create .shader files
                outputFilePath.replace_extension(".shader");
            }
            // TextureImporter keeps the original extension
            
            // Try to delete the output file if it exists
            if (std::filesystem::exists(outputFilePath))
            {
                try
                {
                    std::filesystem::remove(outputFilePath);
                    std::cout << "Deleted output file: " << outputFilePath << std::endl;
                    anyDeleted = true;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to delete output file " << outputFilePath << ": " << e.what() << std::endl;
                }
            }
            
            // Also try to delete any .meta file associated with the output
            std::filesystem::path metaFilePath = outputFilePath;
            metaFilePath += ".meta";
            if (std::filesystem::exists(metaFilePath))
            {
                try
                {
                    std::filesystem::remove(metaFilePath);
                    std::cout << "Deleted meta file: " << metaFilePath << std::endl;
                    anyDeleted = true;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to delete meta file " << metaFilePath << ": " << e.what() << std::endl;
                }
            }
        }
        
        return anyDeleted;
    }

    void cleanupOrphanedFiles(const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config)
    {
        std::cout << "Checking for orphaned output files..." << std::endl;
        
        // Register importers to understand file mappings
        registerImporters(config);
        auto& registry = ResourceImporterRegistry::instance();
        
        // Build a set of all source files that could produce output
        std::unordered_set<std::string> validSourceFiles;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() == ".meta")
                continue;
                
            std::string filePath = entry.path().string();
            auto importers = registry.getImporters(filePath);
            if (!importers.empty())
            {
                validSourceFiles.insert(filePath);
            }
        }
        
        // Scan output directory for files that might be orphaned
        std::vector<std::filesystem::path> filesToDelete;
        
		if (std::filesystem::exists(outputDir))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(outputDir))
			{
				if (!entry.is_regular_file())
					continue;
					
				std::filesystem::path outputFile = entry.path();
				std::string outputFilePath = outputFile.string();
				
				// Skip certain system files
				if (outputFile.extension() == ".db" || outputFile.filename() == ".DS_Store")
					continue;
				
				// Calculate what the corresponding source file would be
				std::filesystem::path relativePath = std::filesystem::relative(outputFile, outputDir);
				
				// Try different possible source file extensions
				std::vector<std::string> possibleSourceExtensions = {
					".glb", ".gltf", ".png", ".jpg", ".jpeg", ".bmp", ".tga", 
					".ttf", ".otf", ".hlsl", ".glsl"
				};
				
				bool hasCorrespondingSource = false;
				
				// For output files with engine extensions, check if source exists
				std::string outputExt = outputFile.extension().string();
				if (outputExt == ".mesh" || outputExt == ".skeleton" || outputExt == ".animation" ||
					outputExt == ".font" || outputExt == ".shader")
				{
					// These are generated files, check for corresponding source
					std::filesystem::path baseSourcePath = std::filesystem::path(sourceDir) / relativePath;
					baseSourcePath.replace_extension(""); // Remove the output extension
					
					for (const std::string& srcExt : possibleSourceExtensions)
					{
						std::filesystem::path possibleSource = baseSourcePath;
						possibleSource += srcExt;
						
						if (validSourceFiles.find(possibleSource.string()) != validSourceFiles.end())
						{
							hasCorrespondingSource = true;
							break;
						}
					}
				}
				else if (outputExt == ".meta")
				{
					// Meta files should have corresponding asset files
					std::filesystem::path assetPath = outputFile;
					assetPath.replace_extension("");
					
					if (std::filesystem::exists(assetPath))
					{
						hasCorrespondingSource = true; // Meta file has corresponding asset
					}
				}
				else
				{
					// For direct copies (like textures), check if source still exists
					std::filesystem::path sourceFile = std::filesystem::path(sourceDir) / relativePath;
					hasCorrespondingSource = validSourceFiles.find(sourceFile.string()) != validSourceFiles.end();
				}
				
				if (!hasCorrespondingSource)
				{
					filesToDelete.push_back(outputFile);
				}
			}
		}
		
        // Delete orphaned files
        for (const auto& fileToDelete : filesToDelete)
        {
            try
            {
                std::filesystem::remove(fileToDelete);
                std::cout << "Removed orphaned file: " << fileToDelete << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to remove orphaned file " << fileToDelete << ": " << e.what() << std::endl;
            }
        }
        
        if (filesToDelete.empty())
        {
            std::cout << "No orphaned files found." << std::endl;
        }
        else
        {
            std::cout << "Cleaned up " << filesToDelete.size() << " orphaned files." << std::endl;
        }
    }
}
