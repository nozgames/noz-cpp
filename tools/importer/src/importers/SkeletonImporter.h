/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "../GLTFLoader.h"

namespace noz::import
{
	class SkeletonImporter : public AssetImporter
	{
	public:
		SkeletonImporter(const ImportConfig::ModelConfig& config);

		bool canImport(const std::string& filePath) const override;
		bool import(const std::string& sourcePath, const std::string& outputDir) override;
		std::vector<std::string> getSupportedExtensions() const override;
		std::string getName() const override;

	private:
		ImportConfig::ModelConfig _config;

		// Process skeleton from GLB file
		bool processSkeleton(const std::string& sourcePath, const std::string& outputPath);

		bool writeSkeleton(
			const std::string& outputPath,
			const std::vector<GLTFLoader::Bone>& bones,
			const std::string& sourcePath);
		
		// Print bone hierarchy for debugging
		void printBoneHierarchy(const std::vector<GLTFLoader::Bone>& bones, const std::string& sourcePath);
	};
}
