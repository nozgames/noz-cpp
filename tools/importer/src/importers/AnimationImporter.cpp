/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "AnimationImporter.h"
#include "../GLTFLoader.h"

using namespace noz::import;

AnimationImporter::AnimationImporter(const ImportConfig::ModelConfig& config)
    : _config(config)
{
}

bool AnimationImporter::canImport(const std::string& filePath) const
{
    // Check if it's a GLB file
    if (filePath.length() < 4 || filePath.substr(filePath.length() - 4) != ".glb")
        return false;
    
    return MetaFile::parse(filePath + ".meta").getBool("Mesh", "importAnimation", false);
}

std::vector<std::string> AnimationImporter::getSupportedExtensions() const
{
    return { ".glb" };
}

std::string AnimationImporter::getName() const
{
    return "AnimationImporter";
}

void AnimationImporter::import(const std::string& sourcePath, const std::string& outputDir)
{
	std::filesystem::path sourceFile(sourcePath);
	std::string fileName = sourceFile.stem().string();
    GLTFLoader loader;
    if (!loader.open(sourcePath))
		throw std::runtime_error("invalid glb file");
    
    auto bones = loader.readBones(GLTFLoader::BoneFilter::fromMetaFile(sourcePath + ".meta"));
	auto animation = loader.readAnimation(bones, fileName);
    loader.close();
    
    if (!animation)
		throw std::runtime_error("no animations found in GLB file");
            
    writeAnimation(outputDir + "/" + fileName + ".animation", animation, sourcePath);
}

bool AnimationImporter::writeAnimation(
	const std::string& outputPath, 
    const std::shared_ptr<GLTFLoader::Animation>& animation,
    const std::string& sourcePath)
{
    try
    {        
        noz::StreamWriter writer;
        writer.writeFileSignature("ANIM");
        writer.writeUInt32(1); // Version
        writer.writeUInt16(animation->frameCount);
		writer.writeUInt16(animation->frameStride);
        writer.writeVector<noz::renderer::AnimationTrack>(animation->tracks);
		writer.writeVector<float>(animation->data);
     
        // Write to file
		std::filesystem::path outputFile(outputPath);
		std::filesystem::create_directories(outputFile.parent_path());
		if (!writer.writeToFile(outputPath))
        {
            noz::Log::error("AnimationImporter", outputPath + ": failed to write animation file");
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
		noz::Log::error("AnimationImporter", outputPath + ": " + e.what());
        return false;
    }
}