/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "ImportConfig.h"
#include <string>

namespace noz::import
{
	bool import(const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config);
	bool importFile(const std::string& filePath, const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config);
	bool deleteOutputFile(const std::string& deletedFilePath, const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config);
	void cleanupOrphanedFiles(const std::string& sourceDir, const std::string& outputDir, const ImportConfig& config);
}