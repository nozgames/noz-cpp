//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <gltf.h>
#include <algorithm>
#include <filesystem>
#include <noz/asset.h>
#include <noz/noz.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace noz;

static void FlattenMesh(GLTFMesh* mesh)
{
    // Create a vector of triangle indices with their max z values
    struct TriangleInfo
    {
        float maxZ;
        uint16_t i0, i1, i2;
    };
    std::vector<TriangleInfo> triangles;

    // Process each triangle (3 consecutive indices)
    for (size_t i = 0; i < mesh->indices.size(); i += 3)
    {
        uint16_t idx0 = mesh->indices[i];
        uint16_t idx1 = mesh->indices[i + 1];
        uint16_t idx2 = mesh->indices[i + 2];

        // Find the maximum y value in this triangle
        float maxZ = std::max({
            -mesh->positions[idx0].y,
            -mesh->positions[idx1].y,
            -mesh->positions[idx2].y
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
    for (size_t t = 0; t < triangles.size(); t++)
    {
        const auto& tri = triangles[t];
        auto ii = t;
        
        mesh->indices[t * 3 + 0] = tri.i0;
        mesh->indices[t * 3 + 1] = tri.i1;
        mesh->indices[t * 3 + 2] = tri.i2;

        mesh->positions[tri.i0].y = ii * 0.001f;
        mesh->positions[tri.i1].y = ii * 0.001f;
        mesh->positions[tri.i2].y = ii * 0.001f;
    }
}

static void WriteMeshData(
    Stream* stream,
    const GLTFMesh* mesh,
    Props* meta)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_MESH;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);

    // header
    bounds3 bounds = to_bounds(mesh->positions.data(), mesh->positions.size());
    WriteBytes(stream, &bounds, sizeof(bounds3));
    WriteU32(stream, static_cast<uint32_t>(mesh->positions.size()));
    WriteU32(stream, static_cast<uint32_t>(mesh->indices.size()));

    // verts
    for (size_t i = 0; i < mesh->positions.size(); ++i)
    {
        mesh_vertex vertex = {};
        vertex.position = mesh->positions[i];
        vertex.uv0 = vec2(0, 0);
        vertex.bone = 0;
        vertex.normal = vec3(0, 1, 0);
        
        if (mesh->normals.size() == mesh->positions.size() && i < mesh->normals.size())
            vertex.normal = mesh->normals[i];
            
        if (mesh->uvs.size() == mesh->positions.size() && i < mesh->uvs.size())
            vertex.uv0 = mesh->uvs[i];
            
        if (mesh->bone_indices.size() == mesh->positions.size() && i < mesh->bone_indices.size())
            vertex.bone = static_cast<float>(mesh->bone_indices[i]);
            
        WriteBytes(stream, &vertex, sizeof(mesh_vertex));
    }
    
    // indices
    WriteBytes(stream, const_cast<uint16_t*>(mesh->indices.data()), mesh->indices.size() * sizeof(uint16_t));
}

void ImportMesh(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    const fs::path& src_path = source_path;
    
    // Check if mesh import is enabled in meta file
    if (meta->GetBool("mesh", "skip_mesh", false))
        return;

    // Load GLTF/GLB file
    GLTFLoader gltf;
    if (!gltf.open(src_path))
    {
        throw std::runtime_error("Failed to open GLTF/GLB file");
    }
    
    // Create bone filter from meta file
    fs::path meta_path = fs::path(src_path.string() + ".meta");
    GLTFBoneFilter bone_filter = gltf.load_bone_filter_from_meta(meta_path);
    
    // Read bones and mesh
    std::vector<GLTFBone> bones = gltf.read_bones(bone_filter);
    GLTFMesh mesh = gltf.read_mesh(bones);
    
    if (mesh.positions.empty())
    {
        throw std::runtime_error("No mesh data found");
    }
    
    // Apply flatten if requested
    if (meta->GetBool("mesh", "flatten", false))
        FlattenMesh(&mesh);

    // Write mesh data to stream
    WriteMeshData(output_stream, &mesh, meta);
}

bool DoesMeshDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // Check if dependency is the meta file for this mesh
    fs::path meta_path = fs::path(source_path.string() + ".meta");
    
    return meta_path == dependency_path;
}

static const char* g_mesh_extensions[] = {
    ".gltf",
    ".glb",
    nullptr
};

static AssetImporterTraits g_mesh_importer_traits = {
    .type_name = "Mesh",
    .type = TYPE_MESH,
    .signature = ASSET_SIGNATURE_MESH,
    .file_extensions = g_mesh_extensions,
    .import_func = ImportMesh,
    .does_depend_on = DoesMeshDependOn
};

AssetImporterTraits* GetMeshImporterTraits()
{
    return &g_mesh_importer_traits;
}