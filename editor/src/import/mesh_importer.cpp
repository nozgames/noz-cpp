//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
// @STL

#include "../asset/atlas_manager.h"

namespace fs = std::filesystem;
using namespace noz;

constexpr Vec2 OUTLINE_COLOR = ColorUV(0, 10);

struct OutlineConfig {
    float width;
    float offset;
    float boundary_taper;
};

static Mesh* ToMeshWithAtlasUVs(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect) {
    // Create a simple quad with UVs mapping into the atlas texture
    // The mesh geometry is pre-rendered to the atlas, so we just need a bounds quad
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 4, 6);

    float depth = 0.01f + 0.99f * (mesh_data->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);

    // Four corners of the mesh bounds
    Vec2 min = mesh_data->bounds.min;
    Vec2 max = mesh_data->bounds.max;

    // Get atlas UVs for each corner
    Vec2 uv_bl = GetAtlasUV(atlas, rect, mesh_data->bounds, {min.x, min.y});
    Vec2 uv_br = GetAtlasUV(atlas, rect, mesh_data->bounds, {max.x, min.y});
    Vec2 uv_tr = GetAtlasUV(atlas, rect, mesh_data->bounds, {max.x, max.y});
    Vec2 uv_tl = GetAtlasUV(atlas, rect, mesh_data->bounds, {min.x, max.y});

    // Add quad vertices (bottom-left, bottom-right, top-right, top-left)
    AddVertex(builder, {{min.x, min.y}, depth, uv_bl});
    AddVertex(builder, {{max.x, min.y}, depth, uv_br});
    AddVertex(builder, {{max.x, max.y}, depth, uv_tr});
    AddVertex(builder, {{min.x, max.y}, depth, uv_tl});

    // Two triangles for the quad
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);

    Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, mesh_data->name, false);
    Free(builder);

    // Set animation info from atlas rect (use default 12 fps for animated meshes)
    if (rect.frame_count > 1) {
        // Calculate frame_width_uv: full frame width in UV space (including padding)
        // This ensures the shader shifts by the correct amount between frames
        float frame_width_pixels = static_cast<float>(rect.width) / static_cast<float>(rect.frame_count);
        float frame_width_uv = frame_width_pixels / static_cast<float>(atlas->impl->width);
        SetAnimationInfo(mesh, rect.frame_count, 12, frame_width_uv);
    }

    return mesh;
}

// Check if mesh has vertices weighted to multiple bones
static bool HasMultipleBoneWeights(MeshData* mesh) {
    if (!mesh || !mesh->impl) return false;

    MeshFrameData* frame = &mesh->impl->frames[0];
    for (int v = 0; v < frame->vertex_count; v++) {
        const VertexData& vertex = frame->vertices[v];

        int bone_count = 0;
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            if (vertex.weights[w].weight > F32_EPSILON) {
                bone_count++;
                if (bone_count > 1) return true;  // Multiple bones
            }
        }
    }
    return false;
}

// Create triangulated mesh with atlas UVs (for multi-bone meshes that need skinning)
static Mesh* ToMeshWithAtlasUVsTriangulated(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect) {
    // Get the triangulated mesh with bone weights
    Mesh* tri_mesh = ToMesh(mesh_data, false, false);
    if (!tri_mesh) return nullptr;

    // Create new mesh builder to rebuild with atlas UVs
    u16 vertex_count = GetVertexCount(tri_mesh);
    u16 index_count = GetIndexCount(tri_mesh);

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, vertex_count, index_count);

    const MeshVertex* src_vertices = GetVertices(tri_mesh);
    const u16* src_indices = GetIndices(tri_mesh);

    // Remap each vertex's UV to atlas UV
    for (int i = 0; i < vertex_count; i++) {
        MeshVertex v = src_vertices[i];
        // Replace UV with atlas UV based on vertex position
        v.uv = GetAtlasUV(atlas, rect, mesh_data->bounds, v.position);
        AddVertex(builder, v);

        // Copy bone weights
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            if (v.bone_weights[w] > F32_EPSILON) {
                AddVertexWeight(builder, v.bone_indices[w], v.bone_weights[w]);
            }
        }
    }

    // Copy indices
    for (int i = 0; i < index_count; i += 3) {
        AddTriangle(builder, src_indices[i], src_indices[i+1], src_indices[i+2]);
    }

    Mesh* result = CreateMesh(ALLOCATOR_DEFAULT, builder, mesh_data->name, false);
    Free(builder);
    Free(tri_mesh);

    // Set animation info for animated meshes
    if (rect.frame_count > 1) {
        float frame_width_pixels = static_cast<float>(rect.width) / static_cast<float>(rect.frame_count);
        float frame_width_uv = frame_width_pixels / static_cast<float>(atlas->impl->width);
        SetAnimationInfo(result, rect.frame_count, 12, frame_width_uv);
    }

    return result;
}

static void ImportMesh(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* mesh_data = static_cast<MeshData*>(a);

    // Note: Atlas assignment happens during mesh save, not import.
    // This prevents infinite loops when creating new atlas files triggers reimport.

    Mesh* m = nullptr;

    // Check if mesh is in an atlas
    AtlasRect* rect = nullptr;
    AtlasData* atlas = FindAtlasForMesh(mesh_data->name, &rect);
    if (atlas && rect) {
        // Check if mesh has multi-bone weights - if so, keep triangulated geometry
        if (HasMultipleBoneWeights(mesh_data)) {
            m = ToMeshWithAtlasUVsTriangulated(mesh_data, atlas, *rect);
        } else {
            m = ToMeshWithAtlasUVs(mesh_data, atlas, *rect);
        }
    }

    // Fall back to normal mesh if no atlas or atlas mesh creation failed
    if (!m) {
        m = ToMesh(mesh_data, false);
    }

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_MESH;
    header.version = 1;

    Stream* stream = CreateStream(nullptr, 4096);
    WriteAssetHeader(stream, &header);
    SerializeMesh(m, stream);
    SaveStream(stream, path);
    Free(stream);
}

AssetImporter GetMeshImporter() {
    return {
        .type = ASSET_TYPE_MESH,
        .ext = ".mesh",
        .import_func = ImportMesh
    };
}

