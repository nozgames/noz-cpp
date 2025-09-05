//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include "../gltf.h"
#include "../../../src/internal.h"

namespace fs = std::filesystem;

using namespace noz;

struct OutlineConfig
{
    float width = 0.1f;          // Base outline width
    float offset = 0.0f;         // Offset: 0=centered, -1=inside, 1=outside
    float boundary_taper = 0.5f; // Width multiplier for boundary edges
};

void GenerateMeshOutline(GLTFMesh* mesh, const OutlineConfig& config = {});
void MyGenerateMeshOutline(GLTFMesh* mesh, const OutlineConfig& config);

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
            return a.maxZ < b.maxZ;
        });

    // Rebuild the indices array with sorted triangles
    for (size_t t = 0; t < triangles.size(); t++)
    {
        const auto& tri = triangles[t];
        auto ii = t;
        
        mesh->indices[t * 3 + 0] = tri.i0;
        mesh->indices[t * 3 + 1] = tri.i1;
        mesh->indices[t * 3 + 2] = tri.i2;
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
    if (meta->GetBool("mesh", "flatten", config->GetBool("mesh.defaults", "flatten", false)))
        FlattenMesh(&mesh);

    MyGenerateMeshOutline(&mesh, {
        .width = meta->GetFloat("mesh", "outline_width", config->GetFloat("mesh.defaults", "outline_width", 0.01f)),
        .offset = meta->GetFloat("mesh", "outline_offset", config->GetFloat("mesh.defaults", "outline_offset", 0.5f)),
        .boundary_taper = meta->GetFloat("mesh", "outline_boundary_taper", config->GetFloat("mesh.defaults", "outline_boundary_taper", 0.01f))
    });

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

struct Edge
{
    uint16_t v0, v1;
    int triangle_index;
    
    bool operator<(const Edge& other) const
    {
        if (v0 != other.v0) return v0 < other.v0;
        return v1 < other.v1;
    }
    
    bool operator==(const Edge& other) const
    {
        return v0 == other.v0 && v1 == other.v1;
    }
};

constexpr Vec2 ColorUV(int col, int row = 0)
{
    float u = (col * 8 + 4) / 1024.0f;
    float v = (row * 8 + 4) / 1024.0f;
    return Vec2(u, v);
}

constexpr Vec2 OUTLINE_COLOR = ColorUV(10, 0);


struct MyEdge
{
    int count;
    u16 i0;
    u16 i1;
    u64 vh0;
    u64 vh1;
};

static u64 GetVertHash(const Vec3& pos)
{
    return Hash(&pos, sizeof(pos));
}

static u64 GetEdgeHash(const Vec3& p0, const Vec3& p1)
{
    u64 h0 = GetVertHash(p0);
    u64 h1 = GetVertHash(p1);
    return Hash(Min(h0,h1), Max(h0,h1));
}

static Vec2 CalculateEdgeDirection(const Vec3& p0, const Vec3& p1)
{
    Vec2 edge_dir = {p1.x - p0.x, p1.y - p0.y};
    float edge_length = sqrt(edge_dir.x * edge_dir.x + edge_dir.y * edge_dir.y);
    
    if (edge_length < 1e-6f)
        return {0.0f, 0.0f}; // Return zero vector for degenerate edges
        
    return {edge_dir.x / edge_length, edge_dir.y / edge_length};
}

static Vec2 CalculateEdgeNormal(const Vec2& edge_dir)
{
    return {-edge_dir.y, edge_dir.x};
}

static Vec2 AverageNormals(const Vec2& normal1, const Vec2& normal2)
{
    Vec2 avg = {(normal1.x + normal2.x) * 0.5f, (normal1.y + normal2.y) * 0.5f};
    float len = sqrt(avg.x * avg.x + avg.y * avg.y);
    
    if (len < 1e-6f)
        return normal1; // Fallback to first normal if averaging results in zero
        
    return {avg.x / len, avg.y / len};
}

static Vec2 CalculateNeighborNormal(const MyEdge& neighbor, u64 shared_vertex, const std::map<u64, Vec3>& vert_map)
{
    Vec3 e_p0 = vert_map.at(neighbor.vh0);
    Vec3 e_p1 = vert_map.at(neighbor.vh1);
    
    Vec2 dir;
    if (neighbor.vh0 == shared_vertex) {
        dir = CalculateEdgeDirection(e_p0, e_p1);
    } else {
        dir = CalculateEdgeDirection(e_p1, e_p0);
    }
    
    return CalculateEdgeNormal(dir);
}

static void AddOutlineVertices(GLTFMesh* mesh, const Vec3& p0, const Vec3& p1, 
                              const Vec2& normal_p0, const Vec2& normal_p1,
                              float width_p0, float width_p1, float offset)
{
    float inner_offset_p0 = -width_p0 * (0.5f + offset * 0.5f);
    float outer_offset_p0 = width_p0 * (0.5f - offset * 0.5f);
    float inner_offset_p1 = -width_p1 * (0.5f + offset * 0.5f);
    float outer_offset_p1 = width_p1 * (0.5f - offset * 0.5f);
    
    // Add vertices for outline quad
    mesh->positions.push_back({
        p0.x + normal_p0.x * inner_offset_p0,
        p0.y + normal_p0.y * inner_offset_p0,
        p0.z
    });
    mesh->positions.push_back({
        p0.x + normal_p0.x * outer_offset_p0,
        p0.y + normal_p0.y * outer_offset_p0,
        p0.z
    });
    mesh->positions.push_back({
        p1.x + normal_p1.x * inner_offset_p1,
        p1.y + normal_p1.y * inner_offset_p1,
        p1.z
    });
    mesh->positions.push_back({
        p1.x + normal_p1.x * outer_offset_p1,
        p1.y + normal_p1.y * outer_offset_p1,
        p1.z
    });
}

static void AddOutlineAttributes(GLTFMesh* mesh, const Vec2& normal_p0, const Vec2& normal_p1)
{
    // Copy normals if they exist
    if (!mesh->normals.empty())
    {
        Vec3 outline_normal_p0 = {normal_p0.x, normal_p0.y, 0.0f};
        Vec3 outline_normal_p1 = {normal_p1.x, normal_p1.y, 0.0f};
        mesh->normals.push_back(outline_normal_p0);
        mesh->normals.push_back(outline_normal_p0);
        mesh->normals.push_back(outline_normal_p1);
        mesh->normals.push_back(outline_normal_p1);
    }
    
    // Add UVs for outline vertices
    if (!mesh->uvs.empty())
    {
        mesh->uvs.push_back(OUTLINE_COLOR);
        mesh->uvs.push_back(OUTLINE_COLOR);
        mesh->uvs.push_back(OUTLINE_COLOR);
        mesh->uvs.push_back(OUTLINE_COLOR);
    }
}

static void AddOutlineTriangles(GLTFMesh* mesh, uint16_t base_vertex_index)
{
    uint16_t v0_inner = base_vertex_index;
    uint16_t v0_outer = base_vertex_index + 1;
    uint16_t v1_inner = base_vertex_index + 2;
    uint16_t v1_outer = base_vertex_index + 3;
    
    // Add triangles for outline quad (2 triangles per edge)
    mesh->indices.push_back(v0_inner);
    mesh->indices.push_back(v0_outer);
    mesh->indices.push_back(v1_inner);
    
    mesh->indices.push_back(v0_outer);
    mesh->indices.push_back(v1_outer);
    mesh->indices.push_back(v1_inner);
}

void MyGenerateMeshOutline(GLTFMesh* mesh, const OutlineConfig& config)
{
    std::map<u64, Vec3> vert_map;
    std::map<u64, MyEdge> edge_map;

    for (size_t i = 0; i < mesh->indices.size(); i += 3)
    {
        u16 i0 = mesh->indices[i];
        u16 i1 = mesh->indices[i + 1];
        u16 i2 = mesh->indices[i + 2];

        const Vec3& p0 = mesh->positions[i0];
        const Vec3& p1 = mesh->positions[i1];
        const Vec3& p2 = mesh->positions[i2];

        u64 ph0 = GetVertHash(p0);
        u64 ph1 = GetVertHash(p1);
        u64 ph2 = GetVertHash(p2);

        vert_map[ph0] = {p0};
        vert_map[ph1] = {p1};
        vert_map[ph2] = {p2};

        u64 eh0 = GetEdgeHash(p0, p1);
        u64 eh1 = GetEdgeHash(p1, p2);
        u64 eh2 = GetEdgeHash(p2, p0);

        MyEdge& e0 = edge_map[eh0];
        e0.i0 = i0;
        e0.i1 = i1;
        e0.vh0 = ph0;
        e0.vh1 = ph1;
        e0.count++;

        MyEdge& e1 = edge_map[eh1];
        e1.i0 = i1;
        e1.i1 = i2;
        e1.vh0 = ph1;
        e1.vh1 = ph2;
        e1.count++;

        MyEdge& e2 = edge_map[eh2];
        e2.i0 = i2;
        e2.i1 = i0;
        e2.vh0 = ph2;
        e2.vh1 = ph0;
        e2.count++;
    }

    for (auto& edge : edge_map)
    {
        if (edge.second.count > 1)
            continue;

        MyEdge n0 = {};
        MyEdge n1 = {};

        // Find neighbors that share vertices with this edge
        for (auto& edge2 : edge_map)
        {
            if (edge2.second.count > 1)
                continue; // Skip non-boundary edges
                
            if (edge2.first == edge.first)
                continue; // Skip self
            
            // Check if edge2 shares the second vertex of the current edge (vh1)
            if (edge2.second.vh0 == edge.second.vh1 ||
                edge2.second.vh1 == edge.second.vh1)
            {
                if (n0.count == 0) // Only take first neighbor found
                    n0 = edge2.second;
            }
            // Check if edge2 shares the first vertex of the current edge (vh0)
            else if (edge2.second.vh0 == edge.second.vh0 ||
                     edge2.second.vh1 == edge.second.vh0)
            {
                if (n1.count == 0) // Only take first neighbor found
                    n1 = edge2.second;
            }
        }

        // All boundary edges should get outlines, but we adjust width based on neighbors

        Vec3 e0p0 = vert_map[edge.second.vh0];
        Vec3 e0p1 = vert_map[edge.second.vh1];
        
        // Calculate main edge direction and normal
        Vec2 edge_dir = CalculateEdgeDirection(e0p0, e0p1);
        if (edge_dir.x == 0.0f && edge_dir.y == 0.0f)
            continue; // Skip degenerate edges
            
        Vec2 edge_normal = CalculateEdgeNormal(edge_dir);
        
        Vec2 normal_p0 = edge_normal;
        Vec2 normal_p1 = edge_normal;
        float width_p0 = config.width;
        float width_p1 = config.width;

        // Both edge points have neighbors
        if (n0.count > 0 && n1.count > 0)
        {
            Vec2 n0_normal = CalculateNeighborNormal(n0, edge.second.vh1, vert_map);
            Vec2 n1_normal = CalculateNeighborNormal(n1, edge.second.vh0, vert_map);
            
            normal_p1 = AverageNormals(edge_normal, n0_normal);
            normal_p0 = AverageNormals(edge_normal, n1_normal);
        }
        // Only one edge point has a neighbor
        else if (n0.count > 0)
        {
            Vec2 n0_normal = CalculateNeighborNormal(n0, edge.second.vh1, vert_map);
            normal_p1 = AverageNormals(edge_normal, n0_normal);
            width_p0 *= config.boundary_taper; // Taper the endpoint without neighbor
        }
        // Only one edge point has a neighbor  
        else if (n1.count > 0)
        {
            Vec2 n1_normal = CalculateNeighborNormal(n1, edge.second.vh0, vert_map);
            normal_p0 = AverageNormals(edge_normal, n1_normal);
            width_p1 *= config.boundary_taper; // Taper the endpoint without neighbor
        }
        else
        {
            // No neighbors - use tapered width for both endpoints
            width_p0 *= config.boundary_taper;
            width_p1 *= config.boundary_taper;
        }
        
        // Generate outline geometry
        uint16_t base_vertex = static_cast<uint16_t>(mesh->positions.size());
        
        AddOutlineVertices(mesh, e0p0, e0p1, normal_p0, normal_p1, width_p0, width_p1, config.offset);
        AddOutlineAttributes(mesh, normal_p0, normal_p1);
        AddOutlineTriangles(mesh, base_vertex);
    }
}

void GenerateMeshOutline(GLTFMesh* mesh, const OutlineConfig& config)
{
    if (mesh->indices.size() % 3 != 0)
        return; // Invalid triangle mesh
    
    // Step 1: Find all edges and count their occurrences
    std::map<Edge, std::vector<int>> edge_triangles;
    
    for (size_t i = 0; i < mesh->indices.size(); i += 3)
    {
        int triangle_idx = static_cast<int>(i / 3);
        uint16_t v0 = mesh->indices[i];
        uint16_t v1 = mesh->indices[i + 1];
        uint16_t v2 = mesh->indices[i + 2];
        
        // Add all three edges of the triangle (ensure consistent ordering)
        Edge edges[3] = {
            {std::min(v0, v1), std::max(v0, v1), triangle_idx},
            {std::min(v1, v2), std::max(v1, v2), triangle_idx},
            {std::min(v2, v0), std::max(v2, v0), triangle_idx}
        };
        
        for (const Edge& edge : edges)
        {
            edge_triangles[{edge.v0, edge.v1, -1}].push_back(triangle_idx);
        }
    }
    
    // Step 2: Find boundary edges (edges that belong to only one triangle)
    std::vector<Edge> boundary_edges;
    for (const auto& [edge_key, triangles] : edge_triangles)
    {
        if (triangles.size() == 1)
        {
            boundary_edges.push_back({edge_key.v0, edge_key.v1, triangles[0]});
        }
    }
    
    if (boundary_edges.empty())
        return; // No boundary edges found
    
    // Step 3: Calculate edge normals and generate outline vertices
    size_t original_vertex_count = mesh->positions.size();
    size_t original_index_count = mesh->indices.size();
    
    // Reserve space for outline vertices (2 vertices per boundary edge)
    mesh->positions.reserve(original_vertex_count + boundary_edges.size() * 2);
    if (!mesh->normals.empty())
        mesh->normals.reserve(original_vertex_count + boundary_edges.size() * 2);
    if (!mesh->uvs.empty())
        mesh->uvs.reserve(original_vertex_count + boundary_edges.size() * 2);
    
    // Reserve space for outline indices (6 indices per boundary edge = 2 triangles)
    mesh->indices.reserve(original_index_count + boundary_edges.size() * 6);
    
    for (const Edge& edge : boundary_edges)
    {
        Vec3 p0 = mesh->positions[edge.v0];
        Vec3 p1 = mesh->positions[edge.v1];
        
        // Calculate edge direction (in 2D, ignoring Z)
        Vec2 edge_dir = {p1.x - p0.x, p1.y - p0.y};
        float edge_length = sqrt(edge_dir.x * edge_dir.x + edge_dir.y * edge_dir.y);
        
        if (edge_length < 1e-6f)
            continue; // Skip degenerate edges
        
        edge_dir.x /= edge_length;
        edge_dir.y /= edge_length;
        
        // Calculate perpendicular direction (outward normal)
        Vec2 perp = {-edge_dir.y, edge_dir.x};
        
        // Try to use mesh normals if available for better outline direction
        Vec2 avg_normal = perp;
        if (!mesh->normals.empty() && edge.v0 < mesh->normals.size() && edge.v1 < mesh->normals.size())
        {
            Vec3 n0 = mesh->normals[edge.v0];
            Vec3 n1 = mesh->normals[edge.v1];
            Vec2 normal_2d = {(n0.x + n1.x) * 0.5f, (n0.y + n1.y) * 0.5f};
            float normal_len = sqrt(normal_2d.x * normal_2d.x + normal_2d.y * normal_2d.y);
            if (normal_len > 1e-6f)
            {
                avg_normal = {normal_2d.x / normal_len, normal_2d.y / normal_len};
            }
        }
        
        // Calculate outline width for this edge
        float outline_width = config.width * config.boundary_taper;
        
        // Calculate offset positions
        float inner_offset = -outline_width * (0.5f + config.offset * 0.5f);
        float outer_offset = outline_width * (0.5f - config.offset * 0.5f);
        
        // Generate outline vertices
        uint16_t v0_inner = static_cast<uint16_t>(mesh->positions.size());
        uint16_t v0_outer = v0_inner + 1;
        uint16_t v1_inner = v0_outer + 1;
        uint16_t v1_outer = v1_inner + 1;
        
        // Add vertices for outline quad
        mesh->positions.push_back({
            p0.x + avg_normal.x * inner_offset,
            p0.y + avg_normal.y * inner_offset,
            p0.z
        });
        mesh->positions.push_back({
            p0.x + avg_normal.x * outer_offset,
            p0.y + avg_normal.y * outer_offset,
            p0.z
        });
        mesh->positions.push_back({
            p1.x + avg_normal.x * inner_offset,
            p1.y + avg_normal.y * inner_offset,
            p1.z
        });
        mesh->positions.push_back({
            p1.x + avg_normal.x * outer_offset,
            p1.y + avg_normal.y * outer_offset,
            p1.z
        });
        
        // Copy normals if they exist
        if (!mesh->normals.empty())
        {
            Vec3 outline_normal = {avg_normal.x, avg_normal.y, 0.0f};
            mesh->normals.push_back(outline_normal);
            mesh->normals.push_back(outline_normal);
            mesh->normals.push_back(outline_normal);
            mesh->normals.push_back(outline_normal);
        }
        
        // Copy UVs if they exist (interpolate along edge)
        if (!mesh->uvs.empty() && edge.v0 < mesh->uvs.size() && edge.v1 < mesh->uvs.size())
        {
            Vec2 uv0 = mesh->uvs[edge.v0];
            Vec2 uv1 = mesh->uvs[edge.v1];
            mesh->uvs.push_back(OUTLINE_COLOR);
            mesh->uvs.push_back(OUTLINE_COLOR);
            mesh->uvs.push_back(OUTLINE_COLOR);
            mesh->uvs.push_back(OUTLINE_COLOR);
        }
        else if (!mesh->uvs.empty())
        {
            // Add default UVs if UV array exists but indices are out of range
            mesh->uvs.push_back({0.0f, 0.0f});
            mesh->uvs.push_back({0.0f, 0.0f});
            mesh->uvs.push_back({0.0f, 0.0f});
            mesh->uvs.push_back({0.0f, 0.0f});
        }
        
        // Add triangles for outline quad (2 triangles per edge)
        // Triangle 1: inner0, outer0, inner1
        mesh->indices.push_back(v0_inner);
        mesh->indices.push_back(v0_outer);
        mesh->indices.push_back(v1_inner);
        
        // Triangle 2: outer0, outer1, inner1
        mesh->indices.push_back(v0_outer);
        mesh->indices.push_back(v1_outer);
        mesh->indices.push_back(v1_inner);
    }
}
