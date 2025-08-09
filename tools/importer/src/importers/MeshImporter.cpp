#include "MeshImporter.h"
#include "../GLTFLoader.h"
#include <algorithm>

using namespace noz::import;

MeshImporter::MeshImporter(const ImportConfig::ModelConfig& config)
    : _config(config)
{
}

bool MeshImporter::canImport(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    bool isValidExtension = extension == ".gltf" || extension == ".glb";
    if (!isValidExtension)
        return false;
    
    return MetaFile::parse(filePath + ".meta").getBool("Mesh", "importMesh", true);
}

bool MeshImporter::import(const std::string& sourcePath, const std::string& outputDir)
{
    try
    {
        std::filesystem::path source(sourcePath);
        std::filesystem::path output(outputDir);
        
        // Create output filename
        std::string outputName = source.stem().string() + ".mesh";
        std::filesystem::path outputPath = output / outputName;
        
        return processMesh(sourcePath, outputPath.string());
    }
    catch (const std::exception& e)
    {
        noz::Log::error("MeshImporter", std::string("Failed to import ") + sourcePath + ": " + e.what());
        return false;
    }
}

std::vector<std::string> MeshImporter::getSupportedExtensions() const
{
    return {".gltf", ".glb"};
}

std::string MeshImporter::getName() const
{
    return "MeshImporter";
}

bool MeshImporter::processMesh(const std::string& sourcePath, const std::string& outputPath)
{
    // Load GLB file
    GLTFLoader loader;
    if (!loader.open(sourcePath))
    {
        noz::Log::error("MeshImporter", "Failed to open GLB file: " + sourcePath);
        return false;
    }
    
    auto bones = loader.readBones(GLTFLoader::BoneFilter::fromMetaFile(sourcePath + ".meta"));
    auto meshData = loader.readMesh(bones);
    
    loader.close();
    
    if (meshData.positions.empty())
    {
        noz::Log::warning("MeshImporter", "No mesh data found in file: " + sourcePath);
        return false;
    }

	auto meta = MetaFile::parse(sourcePath + ".meta");

	if (meta.getBool("Mesh", "flatten", false))
		flatten(meshData);
    
    // Save mesh data
    return saveMeshData(outputPath, meshData, meta);
}

void MeshImporter::flatten(GLTFLoader::MeshData& meshData)
{
	// Create a vector of triangle indices with their max z values
	struct TriangleInfo
	{
		float maxZ;
		uint16_t i0, i1, i2;
	};
	std::vector<TriangleInfo> triangles;
	
	// Process each triangle (3 consecutive indices)
	for (size_t i = 0; i < meshData.indices.size(); i += 3)
	{
		uint16_t idx0 = meshData.indices[i];
		uint16_t idx1 = meshData.indices[i + 1];
		uint16_t idx2 = meshData.indices[i + 2];
		
		// Find the maximum z value in this triangle
		float maxZ = std::max({
			-meshData.positions[idx0].z,
			-meshData.positions[idx1].z,
			-meshData.positions[idx2].z
		});
		
		triangles.push_back({maxZ, idx0, idx1, idx2});
	}
	
	// Sort triangles by max z value (back to front - highest z first)
	std::sort(triangles.begin(), triangles.end(), 
		[](const TriangleInfo& a, const TriangleInfo& b)
		{
			return a.maxZ > b.maxZ;
		});
	
	// Rebuild the indices array with sorted triangles
	meshData.indices.clear();
	for (const auto& tri : triangles)
	{
		auto ii = meshData.indices.size() / 3;
		meshData.indices.push_back(tri.i0);
		meshData.indices.push_back(tri.i1);
		meshData.indices.push_back(tri.i2);

		meshData.positions[tri.i0].z = ii * 0.001f;
		meshData.positions[tri.i1].z = ii * 0.001f;
		meshData.positions[tri.i2].z = ii * 0.001f;
	}
	
	// Now flatten all z values to zero
	//for (auto& position : meshData.positions)
	//	position.z = 0.0f;
}

bool MeshImporter::saveMeshData(const std::string& outputPath, const GLTFLoader::MeshData& meshData, const MetaFile& meta)
{
	auto gpu = meta.getBool("Mesh", "gpu", true);
    auto cpu = meta.getBool("Mesh", "cpu", false);

    try
    {
        // Ensure output directory exists
        std::filesystem::path outputFile(outputPath);
        std::filesystem::create_directories(outputFile.parent_path());
        
        // Create binary writer
        noz::StreamWriter writer;
        
        // Write file signature (no version - reader doesn't expect it)
        writer.writeFileSignature("MESH");
        writer.writeBool(gpu);
        writer.writeBool(cpu);

        // Calculate and write bounds
        noz::bounds3 bounds = noz::bounds3::from_vertices(meshData.positions);
        writer.writeFloat(bounds.min().x);
        writer.writeFloat(bounds.min().y);
        writer.writeFloat(bounds.min().z);
        writer.writeFloat(bounds.max().x);
        writer.writeFloat(bounds.max().y);
        writer.writeFloat(bounds.max().z);

        // Write ModelData header
        writer.writeUInt16(static_cast<uint16_t>(meshData.positions.size())); // vertexCount
        writer.writeUInt16(static_cast<uint16_t>(meshData.indices.size()));   // indexCount
        writer.writeBool(!meshData.normals.empty());     // hasNormals
        writer.writeBool(!meshData.uvs.empty());         // hasUVs
        writer.writeBool(!meshData.boneIndices.empty()); // hasBoneIndices
        writer.writeBool(false);                // hasAnimations (not supported in mesh files)
        writer.writeUInt8(0); // padding[0]
        writer.writeUInt8(0); // padding[1]
        
        // Write vertex data (positions)
        for (const auto& pos : meshData.positions)
        {
            writer.writeFloat(pos.x);
            writer.writeFloat(pos.y);
            writer.writeFloat(pos.z);
        }
        
        // Write normal data (if present)
        if (!meshData.normals.empty())
        {
            for (const auto& normal : meshData.normals)
            {
                writer.writeFloat(normal.x);
                writer.writeFloat(normal.y);
                writer.writeFloat(normal.z);
            }
        }
        
        // Write UV data (if present)
        if (!meshData.uvs.empty())
        {
            for (const auto& uv : meshData.uvs)
            {
                writer.writeFloat(uv.x);
                writer.writeFloat(uv.y);
            }
        }
        
        // Write bone indices (if present)
        if (!meshData.boneIndices.empty())
        {
            for (uint32_t boneIndex : meshData.boneIndices)
            {
                writer.writeUInt32(boneIndex);
            }
        }
        
        // Write index data
        for (uint16_t index : meshData.indices)
        {
            writer.writeUInt16(index);
        }
        
        // Write to file
        if (!writer.writeToFile(outputPath))
        {
            noz::Log::error("MeshImporter", "Failed to write mesh file: " + outputPath);
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        noz::Log::error("MeshImporter", "Failed to save mesh data to " + outputPath + ": " + e.what());
        return false;
    }
}