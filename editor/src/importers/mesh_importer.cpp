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

void CreateSDF(const GLTFMesh& mesh, Stream* stream)
{
    std::map<u64,MyColor> colors;

    for (int i=0; i<mesh.indices.size(); i+=3)
    {
        i32 i0 = mesh.indices[i+0];
        Color c0 = mesh.colors[i0];
        u64 ch = Hash(&c0, sizeof(Color));
        colors[ch] = { .color = c0 };
    }

    for (auto& color : colors)
    {
        for (int i=0; i<mesh.indices.size(); i+=3)
        {
            i32 i0 = mesh.indices[i+0];
            i32 i1 = mesh.indices[i+1];
            i32 i2 = mesh.indices[i+2];

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

    Bounds3 bounds = ToBounds(mesh.positions.data(), mesh.positions.size());

    float size_x = bounds.max.x - bounds.min.x;
    float size_y = bounds.max.y - bounds.min.y;

    float scale;
    if (size_x > size_y)
        scale = size_x;
    else
        scale = size_y;

    float padding = scale * 0.1f;

    std::vector <uint8_t> output;
    output.resize(128 * 16 * 128);
    memset(output.data(), 0, output.size());

    std::vector<MeshVertex> vertices;
    std::vector<u16> indices;


    float uvx = 1.0 / 16.0f;
    int offset = 0;
    int uvoffset = 0;
    for (auto& color : colors)
    {
        RenderShape(
            color.second.shape,
            output,
            128 * 16,
            {offset,0},
            {128,128},
            0.05f,
            {scale + padding * 2, scale + padding * 2},
            {
                bounds.min.x - padding,
                bounds.min.y - padding
            }
        );
        offset += 128;

        u16 vindex = (u16)vertices.size();
        vertices.push_back({
            .position = {-0.5f, -0.5f},
            .uv0 = {uvx * uvoffset, 0},
            .normal = {0,1},
            .color = color.second.color
        });
        vertices.push_back({
            .position = { 0.5f, -0.5f},
            .uv0 = {uvx * uvoffset + uvx, 0},
            .normal = {0,1},
            .color = color.second.color
        });
            vertices.push_back({
            .position = { 0.5f,  0.5f},
            .uv0 = {uvx * uvoffset + uvx, 1},
            .normal = {0,1},
            .color = color.second.color
        });
        vertices.push_back({
            .position = {-0.5f,  0.5f},
            .uv0 = {uvx * uvoffset, 1},
            .normal = {0,1},
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

    WriteU16(stream, (u16)vertices.size());
    WriteU16(stream, (u16)indices.size());
    WriteU32(stream, 128 * 16);
    WriteU32(stream, 128);
    WriteBytes(stream, vertices.data(), vertices.size() * sizeof(MeshVertex));
    WriteBytes(stream, indices.data(), indices.size() * sizeof(u16));
    WriteBytes(stream, output.data(), output.size());
}

void GenerateNormalMapWithSDF(
    const GLTFMesh& mesh,
    uint8_t* output,
    int width,
    int height,
    const Bounds3& bounds,
    float padding)
{
    float scale_x = bounds.max.x - bounds.min.x;
    float scale_y = bounds.max.y - bounds.min.y;
    float scale = std::max(scale_x, scale_y);
    
    // For each pixel in the output texture
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Convert pixel coordinates to world space
            float world_x = bounds.min.x - padding + (scale + padding * 2) * (x / (float)width);
            float world_y = bounds.min.y - padding + (scale + padding * 2) * (y / (float)height);
            
            Vec3 sample_point = {world_x, world_y, 0.0f};
            Vec3 closest_normal = {0.5f, 0.5f, 1.0f}; // Default normal pointing up
            float min_distance = FLT_MAX;
            bool inside_mesh = false;
            
            // Find closest triangle and compute normal at that point
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                uint16_t i0 = mesh.indices[i];
                uint16_t i1 = mesh.indices[i + 1];
                uint16_t i2 = mesh.indices[i + 2];
                
                const Vec3& v0 = mesh.positions[i0];
                const Vec3& v1 = mesh.positions[i1];
                const Vec3& v2 = mesh.positions[i2];
                
                // Project triangle to 2D for distance calculation
                Vec2 p = {world_x, world_y};
                Vec2 a = {v0.x, v0.y};
                Vec2 b = {v1.x, v1.y};
                Vec2 c = {v2.x, v2.y};
                
                // Compute barycentric coordinates and distance to triangle
                Vec2 v0_2d = b - a;
                Vec2 v1_2d = c - a;
                Vec2 v2_2d = p - a;
                
                float dot00 = Dot(v0_2d, v0_2d);
                float dot01 = Dot(v0_2d, v1_2d);
                float dot02 = Dot(v0_2d, v2_2d);
                float dot11 = Dot(v1_2d, v1_2d);
                float dot12 = Dot(v1_2d, v2_2d);
                
                float inv_denom = 1 / (dot00 * dot11 - dot01 * dot01);
                float u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
                float v = (dot00 * dot12 - dot01 * dot02) * inv_denom;
                
                Vec2 closest_point;
                float distance;
                
                if (u >= 0 && v >= 0 && u + v <= 1)
                {
                    // Point is inside triangle
                    closest_point = p;
                    distance = 0;
                    inside_mesh = true;
                }
                else
                {
                    // Find closest point on triangle edge
                    // This is simplified - you might want a more accurate edge distance calculation
                    Vec2 edge_points[3] = {a, b, c};
                    distance = FLT_MAX;
                    
                    for (int edge = 0; edge < 3; edge++)
                    {
                        Vec2 edge_start = edge_points[edge];
                        Vec2 edge_end = edge_points[(edge + 1) % 3];
                        
                        Vec2 edge_vec = edge_end - edge_start;
                        Vec2 to_point = p - edge_start;
                        
                        float t = std::max(0.0f, std::min(1.0f, Dot(to_point, edge_vec) / Dot(edge_vec, edge_vec)));
                        Vec2 point_on_edge = edge_start + edge_vec * t;
                        
                        Vec2 to_closest = p - point_on_edge;
                        float edge_distance = sqrt(Dot(to_closest, to_closest));
                        
                        if (edge_distance < distance)
                        {
                            distance = edge_distance;
                            closest_point = point_on_edge;
                        }
                    }
                }
                
                if (distance < min_distance)
                {
                    min_distance = distance;
                    
                    // Compute triangle normal
                    Vec3 edge1 = v1 - v0;
                    Vec3 edge2 = v2 - v0;
                    Vec3 face_normal = Normalize(Cross(edge1, edge2));
                    
                    // Store normal in 0-1 range for texture
                    closest_normal = Vec3{
                        face_normal.x * 0.5f + 0.5f,
                        face_normal.y * 0.5f + 0.5f,
                        face_normal.z * 0.5f + 0.5f
                    };
                }
            }
            
            // Convert distance to SDF value (negative inside, positive outside)
            float sdf_value = inside_mesh ? -min_distance : min_distance;
            
            // Normalize SDF to 0-255 range (128 = surface, <128 = inside, >128 = outside)
            uint8_t sdf_byte = (uint8_t)std::clamp(128.0f + sdf_value * 32.0f, 0.0f, 255.0f);
            
            // Write RGBA pixel: RGB = normal map, A = SDF
            int pixel_index = (y * width + x) * 4;
            output[pixel_index + 0] = (uint8_t)(closest_normal.x * 255.0f); // R
            output[pixel_index + 1] = (uint8_t)(closest_normal.y * 255.0f); // G  
            output[pixel_index + 2] = (uint8_t)(closest_normal.z * 255.0f); // B
            output[pixel_index + 3] = sdf_byte;                             // A
        }
    }
}
