#include "noz_import.h"
#include <noz/tools/FileWatcher.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>

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
	
	// Set up file watchers for each source directory
	std::vector<std::unique_ptr<noz::tools::FileWatcher>> watchers;
	std::mutex importMutex;
	
	for (const std::string& srcDir : sourceDirs)
	{
		if (!std::filesystem::exists(srcDir))
		{
			std::cout << "Skipping non-existent directory: " << srcDir << std::endl;
			continue;
		}
		
		auto watcher = std::make_unique<noz::tools::FileWatcher>();
		bool success = watcher->watchDirectory(srcDir, [&sourceDirs, &outputDir, &importConfig, &importMutex](const std::string& filePath) {
			std::lock_guard<std::mutex> lock(importMutex);
			try
			{
				noz::import::importFileWithDependencies(filePath, sourceDirs, outputDir, importConfig);
			}
			catch (const std::exception& e)
			{
				std::cerr << filePath << ": " << e.what() << std::endl;
			}
		});
		
		if (success)
		{
			std::cout << "Watching directory: " << srcDir << std::endl;
			watchers.push_back(std::move(watcher));
		}
		else
		{
			std::cerr << "Failed to watch directory: " << srcDir << std::endl;
		}
	}

	// Main loop - process file watcher callbacks
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		
		// Update all file watchers to process queued callbacks
		for (auto& watcher : watchers)
		{
			if (watcher && watcher->isWatching())
			{
				watcher->update();
			}
		}
	}

	return 0;
}
