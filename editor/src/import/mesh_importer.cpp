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
    int atlas_idx = GetAtlasIndex(atlas);

    // Get tight geometry (pixel bounds) and UVs from shared function
    Vec2 min, max;
    float u_min, v_min, u_max, v_max;
    GetTightQuadGeometry(atlas, rect, &min, &max, &u_min, &v_min, &u_max, &v_max);

    Vec2 uv_bl = {u_min, v_min};
    Vec2 uv_br = {u_max, v_min};
    Vec2 uv_tr = {u_max, v_max};
    Vec2 uv_tl = {u_min, v_max};

    // Add quad vertices (bottom-left, bottom-right, top-right, top-left)
    AddVertex(builder, {{min.x, min.y}, depth, 1.0f, uv_bl, {}, {}, {}, atlas_idx});
    AddVertex(builder, {{max.x, min.y}, depth, 1.0f, uv_br, {}, {}, {}, atlas_idx});
    AddVertex(builder, {{max.x, max.y}, depth, 1.0f, uv_tr, {}, {}, {}, atlas_idx});
    AddVertex(builder, {{min.x, max.y}, depth, 1.0f, uv_tl, {}, {}, {}, atlas_idx});

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

static Mesh* ToMeshWithAtlasUVsHull(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect, int bone_index) {
    if (!mesh_data || !mesh_data->impl) return nullptr;

    MeshFrameData* frame = &mesh_data->impl->frames[0];

    // Compute convex hull
    int hull_indices[MESH_MAX_VERTICES];
    int hull_count = ComputeConvexHull(mesh_data, hull_indices, MESH_MAX_VERTICES);
    if (hull_count < 3) return nullptr;

    float expand = ATLAS_HULL_EXPAND / (float)atlas->impl->dpi;
    Vec2 expanded[MESH_MAX_VERTICES];
    ExpandHullByEdgeNormals(mesh_data, hull_indices, hull_count, expand, expanded);

    // Triangulate hull as fan from first vertex
    int tri_count = hull_count - 2;
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, (u16)hull_count, (u16)(tri_count * 3));

    float depth = 0.01f + 0.99f * (mesh_data->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);
    int atlas_idx = GetAtlasIndex(atlas);

    // Add hull vertices with atlas UVs and bone weight
    // Use original vertex positions for UVs (stays within valid atlas content)
    // Geometry is expanded but texture stretches slightly at edges
    for (int i = 0; i < hull_count; i++) {
        const VertexData& src = frame->vertices[hull_indices[i]];
        MeshVertex v = {};
        v.position = expanded[i];
        v.depth = depth;
        v.opacity = 1.0f;
        v.uv = GetAtlasUV(atlas, rect, rect.mesh_bounds, src.position);  // UV from original position
        v.atlas_index = atlas_idx;
        v.bone_indices = {bone_index, 0, 0, 0};
        v.bone_weights = {1.0f, 0.0f, 0.0f, 0.0f};
        AddVertex(builder, v);
    }

    // Triangulate as fan
    for (int i = 0; i < tri_count; i++) {
        AddTriangle(builder, 0, (u16)(i + 1), (u16)(i + 2));
    }

    Mesh* result = CreateMesh(ALLOCATOR_DEFAULT, builder, mesh_data->name, false);
    Free(builder);

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
        if (IsSkinnedMesh(mesh_data)) {
            int single_bone = GetSingleBoneIndex(mesh_data);
            if (single_bone >= 0) {
                // Single-bone: use simplified convex hull
                m = ToMeshWithAtlasUVsHull(mesh_data, atlas, *rect, single_bone);
            } else {
                // Multi-bone not supported - warn and export as non-skinned
                LogWarning("Mesh '%s' has multi-bone weights which is not supported, exporting as non-skinned",
                    mesh_data->name->value);
                m = ToMeshWithAtlasUVs(mesh_data, atlas, *rect);
            }
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

