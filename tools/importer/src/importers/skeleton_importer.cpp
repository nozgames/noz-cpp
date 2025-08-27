#if 0
/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/binary_stream.h>
#include "SkeletonImporter.h"
#include "../gltf_loader.h"

using namespace noz;

SkeletonImporter::SkeletonImporter(const ImportConfig::ModelConfig& config)
    : _config(config)
{
}

bool SkeletonImporter::can_import(const string& filePath) const
{
    // Check if it's a GLB file
    if (filePath.length() < 4 || filePath.substr(filePath.length() - 4) != ".glb")
        return false;

    // Check if meta file exists and specifies skeleton import
    string metaPath = filePath + ".meta";
    if (!filesystem::exists(metaPath))
        return false;

    return meta_file::parse(metaPath).get_bool("Mesh", "importSkeleton", false);
}

vector<string> SkeletonImporter::get_supported_extensions() const
{
    return { ".glb" };
}

string SkeletonImporter::get_name() const
{
    return "SkeletonImporter";
}

void SkeletonImporter::import(const string& source_path, const string& outputDir)
{
    filesystem::path sourceFile(source_path);
    string output_path = outputDir + "/" + sourceFile.stem().string() + ".skeleton";

    gltf_loader loader;
    if (!loader.open(source_path))
		throw runtime_error("invalid GLB file");

    auto bones = loader.read_bones(gltf_loader::BoneFilter::fromMetaFile(source_path + ".meta"));
    loader.close();

    if (bones.empty())
		throw runtime_error("No bones found");

    write_skeleton(output_path, bones, source_path);
}

void SkeletonImporter::write_skeleton(
	const string& output_path,
	const vector<gltf_bone>& bones,
	const string& source_path)
{
    filesystem::path outputFile(output_path);
    filesystem::create_directories(outputFile.parent_path());

    binary_stream stream;
    stream.write_signature("SKEL");
    stream.write_uint32(1); // Version
    stream.write_uint32(static_cast<uint32_t>(bones.size()));

    for (const auto& bone : bones)
    {
        stream.write_string(bone.name);
        stream.write_int32(bone.index);
        stream.write_int32(bone.parent_index);
		stream.write<mat4>(bone.local_to_world);
		stream.write<mat4>(bone.world_to_local);
		stream.write<glm::vec3>(bone.position);
		stream.write<glm::quat>(bone.rotation);
		stream.write<glm::vec3>(bone.scale);
        stream.write_float(bone.length);
		stream.write<glm::vec3>(bone.direction);
    }

    stream.save(output_path);
}
#endif
