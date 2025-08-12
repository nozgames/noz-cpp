/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "ImportConfig.h"
#include <string>

namespace noz::import
{
	void import(const std::vector<std::string>& sourceDirs, const std::string& outputDir, const ImportConfig& config);
	void importFile(const std::string& filePath, const std::vector<std::string>& sourceDirs, const std::string& outputDir, const ImportConfig& config);
	bool deleteOutputFile(const std::string& deletedFilePath, const std::vector<std::string>& sourceDirs, const std::string& outputDir, const ImportConfig& config);
	void cleanupOrphanedFiles(const std::vector<std::string>& sourceDirs, const std::string& outputDir, const ImportConfig& config);
}