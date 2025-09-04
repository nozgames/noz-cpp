//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include "../gltf.h"
#include "../../../src/internal.h"
#include "../msdf/msdf.h"
#include "../msdf/shape.h"
#include <algorithm>
#include <cfloat>

namespace fs = std::filesystem;

using namespace noz;

void CreateSDF(const GLTFMesh& mesh, Stream* stream);

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
            mesh->positions[idx0].z,
            mesh->positions[idx1].z,
            mesh->positions[idx2].z
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

        mesh->positions[tri.i0].z = ii * 0.001f;
        mesh->positions[tri.i1].z = ii * 0.001f;
        mesh->positions[tri.i2].z = ii * 0.001f;
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

#if 1
    CreateSDF(*mesh, stream);
#else
    std::vector<Vec2> positions;
    positions.reserve(mesh->positions.size());
    for (const Vec3& pos : mesh->positions)
        positions.push_back({pos.x, -pos.y});

    // header
    Bounds2 bounds = ToBounds(positions.data(), positions.size());
    WriteStruct(stream, bounds);
    WriteU16(stream, static_cast<u16>(mesh->positions.size()));
    WriteU16(stream, static_cast<u16>(mesh->indices.size()));

    // verts
    for (size_t i = 0; i < mesh->positions.size(); ++i)
    {
        MeshVertex vertex = {};
        vertex.position = positions[i];
        vertex.uv0 = {0, 0};
        vertex.bone = 0;
        vertex.normal = {0, 1};
        
        if (mesh->normals.size() == mesh->positions.size() && i < mesh->normals.size())
            vertex.normal = { mesh->normals[i].x, mesh->normals[i].y };
            
        if (mesh->uvs.size() == mesh->positions.size() && i < mesh->uvs.size())
            vertex.uv0 = mesh->uvs[i];
            
        if (mesh->bone_indices.size() == mesh->positions.size() && i < mesh->bone_indices.size())
            vertex.bone = static_cast<float>(mesh->bone_indices[i]);
            
        WriteBytes(stream, &vertex, sizeof(MeshVertex));
    }
    
    // indices
    WriteBytes(stream, const_cast<uint16_t*>(mesh->indices.data()), mesh->indices.size() * sizeof(uint16_t));
#endif
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
    // if (meta->GetBool("mesh", "flatten", config->GetBool("mesh.defaults", "flatten", false)))
    //     FlattenMesh(&mesh);

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
    .signature = ASSET_SIGNATURE_MESH,
    .file_extensions = g_mesh_extensions,
    .import_func = ImportMesh,
    .does_depend_on = DoesMeshDependOn
};

AssetImporterTraits* GetMeshImporterTraits()
{
    return &g_mesh_importer_traits;
}



#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/stb_image_write.h"

struct MyEdge
{
    i32 v0;
    i32 v1;
    i32 count;
};

struct MyColor
{
    Color color;
    std::map<u64, MyEdge> edges;
    msdf::Shape* shape;
};

void GenerateNormalMapWithSDF(const GLTFMesh& mesh, std::vector<uint8_t>& output, 
                               const std::map<u64, MyColor>& colors,
                               const Bounds3& bounds, float padding)
{
    float size_x = bounds.max.x - bounds.min.x;
    float size_y = bounds.max.y - bounds.min.y;
    float scale = (size_x > size_y) ? size_x : size_y;

    // Output buffer: RGBA, 128x128 per color, arranged horizontally
    output.resize(128 * 128 * 4 * colors.size());
    memset(output.data(), 0, output.size());

    // Generate SDF directly to alpha channel using new RenderShape overload
    int offset = 0;
    for (auto& color : colors)
    {
        RenderShape(
            color.second.shape,
            output,
            128 * colors.size() * 4, // RGBA stride
            {offset, 0},
            {128, 128},
            0.05f,
            {scale + padding * 2, scale + padding * 2},
            {
                bounds.min.x - padding,
                bounds.min.y - padding
            },
            4, // Component stride (RGBA = 4 bytes per pixel)
            3  // Component offset (A = 3rd offset, 0-based)
        );
        offset += 128;
    }
}

void CreateSDF(const GLTFMesh& mesh, Stream* stream)
{
    // Group triangles by color hash
    std::map<u64, MyColor> colors;
    for (int i = 0; i < mesh.indices.size(); i += 3)
    {
        i32 i0 = mesh.indices[i + 0];
        Color c0 = mesh.colors[i0];
        u64 ch = Hash(&c0, sizeof(Color));
        colors[ch] = { .color = c0 };
    }

    // Build edge data for each color (for MSDF)
    for (auto& color : colors)
    {
        for (int i = 0; i < mesh.indices.size(); i += 3)
        {
            i32 i0 = mesh.indices[i + 0];
            i32 i1 = mesh.indices[i + 1];
            i32 i2 = mesh.indices[i + 2];

            Vec3 v0 = mesh.positions[i0];
            Vec3 v1 = mesh.positions[i1];
            Vec3 v2 = mesh.positions[i2];

            Color c0 = mesh.colors[i0];
            u64 ch = Hash(&c0, sizeof(Color));

            if (color.first != ch)
                continue;

            i32 v0i = (i32)(v0.x * 10000.0f) ^ (i32)(v0.y * 10000.0f);
            i32 v1i = (i32)(v1.x * 10000.0f) ^ (i32)(v1.y * 10000.0f);
            i32 v2i = (i32)(v2.x * 10000.0f) ^ (i32)(v2.y * 10000.0f);

            auto e0 = (u64)Min(v0i, v1i) + ((u64)Max(v0i, v1i) << 32);
            auto e1 = (u64)Min(v1i, v2i) + ((u64)Max(v1i, v2i) << 32);
            auto e2 = (u64)Min(v2i, v0i) + ((u64)Max(v2i, v0i) << 32);

            auto& ve0 = color.second.edges[e0];
            ve0.v0 = mesh.indices[i + 0];
            ve0.v1 = mesh.indices[i + 1];
            ve0.count++;

            auto& ve1 = color.second.edges[e1];
            ve1.v0 = mesh.indices[i + 1];
            ve1.v1 = mesh.indices[i + 2];
            ve1.count++;

            auto& ve2 = color.second.edges[e2];
            ve2.v0 = mesh.indices[i + 2];
            ve2.v1 = mesh.indices[i + 0];
            ve2.count++;
        }
    }

    // Create MSDF shapes for each color
    for (auto& color : colors)
    {
        color.second.shape = new msdf::Shape();

        msdf::Contour* contour = new msdf::Contour();
        for (auto& e : color.second.edges)
        {
            if (e.second.count > 1)
                continue;

            contour->edges.push_back(
                new msdf::LinearEdge(
                    Vec2Double(mesh.positions[e.second.v1].x, mesh.positions[e.second.v1].y),
                    Vec2Double(mesh.positions[e.second.v0].x, mesh.positions[e.second.v0].y)
                ));
        }

        color.second.shape->contours.push_back(contour);
    }
    
    // Calculate bounds and padding
    Bounds3 bounds = ToBounds(mesh.positions.data(), mesh.positions.size());
    float size_x = bounds.max.x - bounds.min.x;
    float size_y = bounds.max.y - bounds.min.y;
    float scale = (size_x > size_y) ? size_x : size_y;
    float padding = scale * 0.1f;

    // Generate RGBA texture data (RGB = normals, A = SDF using MSDF)
    std::vector<uint8_t> rgba_output;
    GenerateNormalMapWithSDF(mesh, rgba_output, colors, bounds, padding);

    // Create vertex data
    std::vector<MeshVertex> vertices;
    std::vector<u16> indices;

    float uvx = 1.0f / colors.size();
    int uvoffset = 0;
    for (auto& color : colors)
    {
        u16 vindex = (u16)vertices.size();
        vertices.push_back({
            .position = {-0.5f, -0.5f},
            .uv0 = {uvx * uvoffset, 0},
            .normal = {0, 1},
            .color = color.second.color
        });
        vertices.push_back({
            .position = { 0.5f, -0.5f},
            .uv0 = {uvx * uvoffset + uvx, 0},
            .normal = {0, 1},
            .color = color.second.color
        });
        vertices.push_back({
            .position = { 0.5f,  0.5f},
            .uv0 = {uvx * uvoffset + uvx, 1},
            .normal = {0, 1},
            .color = color.second.color
        });
        vertices.push_back({
            .position = {-0.5f,  0.5f},
            .uv0 = {uvx * uvoffset, 1},
            .normal = {0, 1},
            .color = color.second.color
        });

        indices.push_back(vindex + 0);
        indices.push_back(vindex + 1);
        indices.push_back(vindex + 2);

        indices.push_back(vindex + 0);
        indices.push_back(vindex + 2);
        indices.push_back(vindex + 3);

        uvoffset++;
    }

    // Write mesh data with RGBA texture
    WriteU16(stream, (u16)vertices.size());
    WriteU16(stream, (u16)indices.size());
    WriteU32(stream, 128 * colors.size()); // Width: 128 per color section
    WriteU32(stream, 128);                 // Height: 128
    WriteBytes(stream, vertices.data(), vertices.size() * sizeof(MeshVertex));
    WriteBytes(stream, indices.data(), indices.size() * sizeof(u16));
    WriteBytes(stream, rgba_output.data(), rgba_output.size());
}
