#include "noz_import.h"
#include "FileWatcher.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

int main(int argc, char* argv[])
{
	// Parse command line arguments
	cxxopts::Options options("importer", "Import assets and watch for changes");
	options.add_options()
		("h,help", "Print usage")
		("s,source", "Source directory (default: assets)", cxxopts::value<std::string>()->default_value("assets"))
		("n,noz-source", "NoZ engine source directory (default: libs/noz/assets)", cxxopts::value<std::string>()->default_value("libs/noz/assets"))
		("o,output", "Output directory (default: assets)", cxxopts::value<std::string>()->default_value("assets"))
		("p,parallel", "Enable parallel processing", cxxopts::value<bool>()->default_value("false"));

	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
		std::cout << options.help() << std::endl;
		return 0;
	}

	// Get directories from command line
	std::string sourceDir = result["source"].as<std::string>();
	std::string nozSourceDir = result["noz-source"].as<std::string>();
	std::string outputDir = result["output"].as<std::string>();

	// Configure asset import settings
	noz::import::ImportConfig importConfig;
	importConfig.parallelProcessing = result["parallel"].as<bool>();
	importConfig.font.fontSize = 16;
	importConfig.font.textureSize = 1024;
	importConfig.font.enableSDF = true;
	importConfig.font.sdfPadding = 4;
	importConfig.shader.preprocessShaders = true;
	importConfig.shader.validateShaders = true;
	importConfig.shader.includePaths.push_back("assets/shaders");

	std::cout << "Resource Importer" << std::endl;
	std::cout << "=================" << std::endl;
	std::cout << "Game Source: " << sourceDir << std::endl;
	std::cout << "NoZ Source: " << nozSourceDir << std::endl;
	std::cout << "Output: " << outputDir << std::endl;
	std::cout << std::endl;

	// Build list of source directories (game assets override noz assets)
	std::vector<std::string> sourceDirs = { sourceDir, nozSourceDir };
	
	// Clean up orphaned files first
	std::cout << "Cleaning up orphaned files..." << std::endl;
	noz::import::cleanupOrphanedFiles(sourceDirs, outputDir, importConfig);
	std::cout << std::endl;

	// Do initial import pass
	std::cout << "Reimporting..." << std::endl;
	noz::import::import(sourceDirs, outputDir, importConfig);

	// Enter watch mode
	std::cout << std::endl << "Entering watch mode. Press Ctrl+C to exit." << std::endl;
	
	// Store file modification times
	std::unordered_map<std::string, std::filesystem::file_time_type> fileModTimes;
	std::unordered_map<std::string, std::filesystem::file_time_type> metaFileModTimes;

	// Initialize modification times for all files in all source directories
	for (const std::string& srcDir : sourceDirs)
	{
		if (!std::filesystem::exists(srcDir))
			continue;
			
		for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir))
		{
			if (entry.is_regular_file())
			{
				std::string filePath = entry.path().string();
				auto lastWriteTime = entry.last_write_time();
				
				if (entry.path().extension() == ".meta")
				{
					metaFileModTimes[filePath] = lastWriteTime;
				}
				else
				{
					fileModTimes[filePath] = lastWriteTime;
				}
			}
		}
	}

	std::cout << "Monitoring directories (polling mode):" << std::endl;
	for (const std::string& srcDir : sourceDirs)
	{
		std::cout << "  - " << srcDir << std::endl;
	}

	// Watch loop
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Poll every 500ms

		try
		{
			// Collect all files that need to be imported
			std::vector<std::string> filesToImport;
			
			// Check for modified files in all source directories
			for (const std::string& srcDir : sourceDirs)
			{
				if (!std::filesystem::exists(srcDir))
					continue;
					
				for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir))
			{
				if (!entry.is_regular_file())
					continue;

				std::string filePath = entry.path().string();
				auto lastWriteTime = entry.last_write_time();

				if (entry.path().extension() == ".meta")
				{
					// Check if meta file is new or modified
					auto it = metaFileModTimes.find(filePath);
					if (it == metaFileModTimes.end() || it->second != lastWriteTime)
					{
						// Update modification time
						metaFileModTimes[filePath] = lastWriteTime;
						
						// Find the associated resource file (remove .meta extension)
						std::filesystem::path metaPath(filePath);
						std::string resourcePath = metaPath.replace_extension("").string();
						
						// Only add to import list if the resource file exists
						if (std::filesystem::exists(resourcePath))
						{
							// Check if we haven't already added this file
							if (std::find(filesToImport.begin(), filesToImport.end(), resourcePath) == filesToImport.end())
							{
								filesToImport.push_back(resourcePath);
							}
						}
					}
				}
				else
				{
					// Check if regular file is new or modified
					auto it = fileModTimes.find(filePath);
					if (it == fileModTimes.end() || it->second != lastWriteTime)
					{
						// Update modification time
						fileModTimes[filePath] = lastWriteTime;
						
						// Add to import list
						filesToImport.push_back(filePath);
					}
				}
			}
			} // End source directory loop
			
			// Process all collected files
			if (!filesToImport.empty())
			{
				for (const auto& filePath : filesToImport)
				{
					try
					{
						noz::import::importFile(filePath, sourceDirs, outputDir, importConfig);
						std::cout << "imported: " << filePath << std::endl;
					}
					catch (const std::exception& e)
					{
						std::cerr << filePath << ": " << e.what() << std::endl;
					}
				}
			}

			// Check for deleted files
			std::vector<std::string> deletedFiles;
			for (const auto& [filePath, modTime] : fileModTimes)
				if (!std::filesystem::exists(filePath))
					deletedFiles.push_back(filePath);
			
			// Check for deleted meta files
			std::vector<std::string> deletedMetaFiles;
			for (const auto& [filePath, modTime] : metaFileModTimes)
				if (!std::filesystem::exists(filePath))
					deletedMetaFiles.push_back(filePath);

			// Remove deleted files from tracking and clean up output files
			for (const auto& filePath : deletedFiles)
			{
				fileModTimes.erase(filePath);
				
				// Delete corresponding output files
				try
				{
					noz::import::deleteOutputFile(filePath, sourceDirs, outputDir, importConfig);
					std::cout << "deleted '" << filePath << "'" << std::endl;
				}
				catch (const std::exception& e)
				{
					std::cerr << filePath << ": " << e.what() << std::endl;
				}
			}
			
			// Remove deleted meta files from tracking
			for (const auto& filePath : deletedMetaFiles)
				metaFileModTimes.erase(filePath);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error during file watch: " << e.what() << std::endl;
		}
	}

	return 0;
}
