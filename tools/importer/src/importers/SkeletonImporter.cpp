/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "SkeletonImporter.h"
#include "../GLTFLoader.h"
#include <functional>
#include <map>

using namespace noz::import;

SkeletonImporter::SkeletonImporter(const ImportConfig::ModelConfig& config)
    : _config(config)
{
}

bool SkeletonImporter::canImport(const std::string& filePath) const
{
    // Check if it's a GLB file
    if (filePath.length() < 4 || filePath.substr(filePath.length() - 4) != ".glb")
        return false;
    
    // Check if meta file exists and specifies skeleton import
    std::string metaPath = filePath + ".meta";
    if (!std::filesystem::exists(metaPath))
        return false;
    
    return MetaFile::parse(metaPath).getBool("Mesh", "importSkeleton", false);
}

void SkeletonImporter::import(const std::string& sourcePath, const std::string& outputDir)
{
    std::filesystem::path sourceFile(sourcePath);
    std::string outputPath = outputDir + "/" + sourceFile.stem().string() + ".skeleton";
    processSkeleton(sourcePath, outputPath);
}

std::vector<std::string> SkeletonImporter::getSupportedExtensions() const
{
    return { ".glb" };
}

std::string SkeletonImporter::getName() const
{
    return "SkeletonImporter";
}

bool SkeletonImporter::processSkeleton(const std::string& sourcePath, const std::string& outputPath)
{
    GLTFLoader loader;
    if (!loader.open(sourcePath))
		throw std::runtime_error("invalid GLB file");
	
    auto bones = loader.readBones(GLTFLoader::BoneFilter::fromMetaFile(sourcePath + ".meta"));
    loader.close();
    
    if (bones.empty())
		throw std::runtime_error("No bones found");
    
    // Print bone hierarchy
    printBoneHierarchy(bones, sourcePath);
    
    return writeSkeleton(outputPath, bones, sourcePath);
}

bool SkeletonImporter::writeSkeleton(
	const std::string& outputPath, 
	const std::vector<GLTFLoader::Bone>& bones,
	const std::string& sourcePath)
{
    try
    {
        std::filesystem::path outputFile(outputPath);
        std::filesystem::create_directories(outputFile.parent_path());
        
        noz::StreamWriter writer;
        writer.writeFileSignature("SKEL");
        writer.writeUInt32(1); // Version
        writer.writeUInt32(static_cast<uint32_t>(bones.size()));
        
        for (const auto& bone : bones)
        {
            writer.writeString(bone.name);
            writer.writeInt32(bone.index);
            writer.writeInt32(bone.parentIndex);
			writer.write<glm::mat4>(bone.localToWorld);
			writer.write<glm::mat4>(bone.worldToLocal);
			writer.write<glm::vec3>(bone.position);
			writer.write<glm::quat>(bone.rotation);
			writer.write<glm::vec3>(bone.scale);
            writer.writeFloat(bone.length);
			writer.write<glm::vec3>(bone.direction);
        }
        
        if (!writer.writeToFile(outputPath))
			throw std::runtime_error("failed to write skeleton file");

		return true;
    }
    catch (const std::exception& e)
    {
        noz::Log::error("SkeletonImporter", outputPath, e.what());
        return false;
    }
}

void SkeletonImporter::printBoneHierarchy(const std::vector<GLTFLoader::Bone>& bones, const std::string& sourcePath)
{
    noz::Log::info("SkeletonImporter", "===== Bone Hierarchy for: " + sourcePath + " =====");
    noz::Log::info("SkeletonImporter", "Total Bones: " + std::to_string(bones.size()));
    
    // Helper function to print a bone and its children recursively
    std::function<void(int, int)> printBone = [&](int boneIndex, int depth) {
        if (boneIndex < 0 || boneIndex >= static_cast<int>(bones.size()))
            return;
            
        const auto& bone = bones[boneIndex];
        
        // Create indentation based on depth using ASCII characters
        std::string indent;
        for (int i = 0; i < depth; ++i)
        {
            if (i == depth - 1)
                indent += "  +- ";
            else
                indent += "  |  ";
        }
        
        // Print bone information
        std::string boneInfo = (depth == 0 ? "" : indent) + 
            "[" + std::to_string(bone.index) + "] " + bone.name;
        
        // Add transform information
        boneInfo += " (pos: " + 
            std::to_string(bone.position.x) + ", " +
            std::to_string(bone.position.y) + ", " +
            std::to_string(bone.position.z) + ")";
        
        // Add rotation information (show as euler angles for readability)
        glm::vec3 euler = glm::degrees(glm::eulerAngles(bone.rotation));
        boneInfo += " (rot: " +
            std::to_string(euler.x) + "°, " +
            std::to_string(euler.y) + "°, " +
            std::to_string(euler.z) + "°)";
        
        // Add scale information
        boneInfo += " (scale: " +
            std::to_string(bone.scale.x) + ", " +
            std::to_string(bone.scale.y) + ", " +
            std::to_string(bone.scale.z) + ")";
        
        // Add bone length
        boneInfo += " (length: " + std::to_string(bone.length) + ")";
        
        noz::Log::info("SkeletonImporter", boneInfo);
        
        // Find and print all children
        for (size_t i = 0; i < bones.size(); ++i)
        {
            if (bones[i].parentIndex == boneIndex)
            {
                printBone(static_cast<int>(i), depth + 1);
            }
        }
    };
    
    // Find and print root bones (bones with no parent or parent index -1)
    noz::Log::info("SkeletonImporter", "Bone Hierarchy:");
    for (size_t i = 0; i < bones.size(); ++i)
    {
        if (bones[i].parentIndex == -1)
        {
            printBone(static_cast<int>(i), 0);
        }
    }
    
    // Print bone summary
    noz::Log::info("SkeletonImporter", "");
    noz::Log::info("SkeletonImporter", "Bone Summary:");
    
    // Count bones by depth level
    std::map<int, int> depthCounts;
    std::function<void(int, int)> countDepths = [&](int boneIndex, int depth) {
        if (boneIndex < 0 || boneIndex >= static_cast<int>(bones.size()))
            return;
        
        depthCounts[depth]++;
        
        for (size_t i = 0; i < bones.size(); ++i)
        {
            if (bones[i].parentIndex == boneIndex)
            {
                countDepths(static_cast<int>(i), depth + 1);
            }
        }
    };
    
    // Count all bones by depth
    for (size_t i = 0; i < bones.size(); ++i)
    {
        if (bones[i].parentIndex == -1)
        {
            countDepths(static_cast<int>(i), 0);
        }
    }
    
    // Print depth summary
    for (const auto& [depth, count] : depthCounts)
    {
        noz::Log::info("SkeletonImporter", "  Level " + std::to_string(depth) + ": " + std::to_string(count) + " bones");
    }
    
    noz::Log::info("SkeletonImporter", "===== End Bone Hierarchy =====");
}