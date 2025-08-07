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
		("s,source", "Source directory (default: resources)", cxxopts::value<std::string>()->default_value("resources"))
		("o,output", "Output directory (default: resources)", cxxopts::value<std::string>()->default_value("resources"))
		("w,watch", "Watch for file changes", cxxopts::value<bool>()->default_value("true"))
		("p,parallel", "Enable parallel processing", cxxopts::value<bool>()->default_value("false"));

	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
		std::cout << options.help() << std::endl;
		return 0;
	}

	// Get directories from command line
	std::string sourceDir = result["source"].as<std::string>();
	std::string outputDir = result["output"].as<std::string>();
	bool watchMode = result["watch"].as<bool>();

	// Configure asset import settings
	noz::import::ImportConfig importConfig;
	importConfig.parallelProcessing = result["parallel"].as<bool>();
	importConfig.font.fontSize = 16;
	importConfig.font.textureSize = 1024;
	importConfig.font.enableSDF = true;
	importConfig.font.sdfPadding = 4;
	importConfig.shader.preprocessShaders = true;
	importConfig.shader.validateShaders = true;
	importConfig.shader.includePaths.push_back("resources/shaders");

	std::cout << "Resource Importer" << std::endl;
	std::cout << "=================" << std::endl;
	std::cout << "Source: " << sourceDir << std::endl;
	std::cout << "Output: " << outputDir << std::endl;
	std::cout << "Watch mode: " << (watchMode ? "enabled" : "disabled") << std::endl;
	std::cout << std::endl;

	// Clean up orphaned files first
	std::cout << "Cleaning up orphaned files..." << std::endl;
	noz::import::cleanupOrphanedFiles(sourceDir, outputDir, importConfig);
	std::cout << std::endl;

	// Do initial import pass
	std::cout << "Performing initial import pass..." << std::endl;
	if (!noz::import::import(sourceDir, outputDir, importConfig))
	{
		std::cerr << "Initial import failed" << std::endl;
		return 1;
	}
	std::cout << "Initial import completed successfully!" << std::endl;

	// If not in watch mode, exit after initial import
	if (!watchMode)
	{
		return 0;
	}

	// Enter watch mode
	std::cout << "\nEntering watch mode. Press Ctrl+C to exit." << std::endl;
	
	// Store file modification times
	std::unordered_map<std::string, std::filesystem::file_time_type> fileModTimes;
	std::unordered_map<std::string, std::filesystem::file_time_type> metaFileModTimes;

	// Initialize modification times for all files
	for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
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

	std::cout << "Monitoring: " << sourceDir << " (polling mode)" << std::endl;

	// Watch loop
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Poll every 500ms

		try
		{
			// Collect all files that need to be imported
			std::vector<std::string> filesToImport;
			
			// Check for modified files
			for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
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
			
			// Process all collected files
			if (!filesToImport.empty())
			{
				std::cout << "\nProcessing " << filesToImport.size() << " file(s)..." << std::endl;
				
				// TODO: Sort filesToImport based on dependencies when needed
				// For now, process in the order they were collected
				
				for (const auto& filePath : filesToImport)
				{
					std::cout << "Importing: " << filePath << std::endl;
					if (!noz::import::importFile(filePath, sourceDir, outputDir, importConfig))
					{
						std::cerr << "Import failed for: " << filePath << std::endl;
					}
				}
				
				std::cout << "Batch import completed." << std::endl;
			}

			// Check for deleted files
			std::vector<std::string> deletedFiles;
			for (const auto& [filePath, modTime] : fileModTimes)
			{
				if (!std::filesystem::exists(filePath))
				{
					deletedFiles.push_back(filePath);
				}
			}
			
			// Check for deleted meta files
			std::vector<std::string> deletedMetaFiles;
			for (const auto& [filePath, modTime] : metaFileModTimes)
			{
				if (!std::filesystem::exists(filePath))
				{
					deletedMetaFiles.push_back(filePath);
				}
			}

			// Remove deleted files from tracking and clean up output files
			for (const auto& filePath : deletedFiles)
			{
				std::cout << "\nDetected deletion of: " << filePath << std::endl;
				fileModTimes.erase(filePath);
				
				// Delete corresponding output files
				if (!noz::import::deleteOutputFile(filePath, sourceDir, outputDir, importConfig))
				{
					std::cerr << "Failed to clean up output files for: " << filePath << std::endl;
				}
			}
			
			// Remove deleted meta files from tracking
			for (const auto& filePath : deletedMetaFiles)
			{
				metaFileModTimes.erase(filePath);
				// No need to do anything else for deleted meta files
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error during file watch: " << e.what() << std::endl;
		}
	}

	return 0;
}
