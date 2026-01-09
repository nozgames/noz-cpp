//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
// @STL

#include "atlas_manager.h"

namespace fs = std::filesystem;
using namespace noz;

constexpr Vec2 OUTLINE_COLOR = ColorUV(0, 10);

struct OutlineConfig {
    float width;
    float offset;
    float boundary_taper;
};

static Mesh* ToMeshWithAtlasQuad(MeshDocument* m, AtlasDocument* atlas, const AtlasRect& rect) {
    if (!m || !m->impl) return nullptr;

    // Use tight rect around mesh bounds
    Vec2 min_pos = rect.mesh_bounds.min;
    Vec2 max_pos = rect.mesh_bounds.max;

    // 4 vertices, 2 triangles
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 4, 6);

    float depth = 0.01f + 0.99f * (m->impl->depth - MESH_MIN_DEPTH) / (float)(MESH_MAX_DEPTH - MESH_MIN_DEPTH);
    int atlas_idx = GetAtlasIndex(atlas);

    // Get bone index if skinned (single bone only)
    int bone_index = m->impl->bone_index;

    // Quad corners: bottom-left, bottom-right, top-right, top-left
    Vec2 corners[4] = {
        {min_pos.x, min_pos.y},
        {max_pos.x, min_pos.y},
        {max_pos.x, max_pos.y},
        {min_pos.x, max_pos.y}
    };

    for (int i = 0; i < 4; i++) {
        MeshVertex v = {};
        v.position = corners[i];
        v.depth = depth;
        v.opacity = 1.0f;
        v.uv = GetAtlasUV(atlas, rect, rect.mesh_bounds, corners[i]);
        v.atlas_index = atlas_idx;


        AddVertex(builder, v);
    }

    // Two triangles for quad
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);

    Mesh* result = CreateMesh(ALLOCATOR_DEFAULT, builder, m->name, false);
    Free(builder);

    // Set animation info for animated meshes
    if (rect.frame_count > 1) {
        float frame_width_pixels = static_cast<float>(rect.width) / static_cast<float>(rect.frame_count);
        float frame_width_uv = frame_width_pixels / static_cast<float>(atlas->impl->size.x);
        SetAnimationInfo(result, rect.frame_count, 12, frame_width_uv);
    }

    return result;
}

static void ImportMesh(Document* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshDocument* mesh_data = static_cast<MeshDocument*>(a);

    Mesh* m = nullptr;

    // Check if mesh is in an atlas - use tight rect quad
    AtlasRect* rect = nullptr;
    AtlasDocument* atlas = FindAtlasForMesh(mesh_data->name, &rect);
    if (atlas && rect) {
        m = ToMeshWithAtlasQuad(mesh_data, atlas, *rect);
    }

    // Fall back to normal mesh if no atlas or pixel hull failed
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

DocumentImporter GetMeshImporter() {
    return {
        .type = ASSET_TYPE_MESH,
        .ext = ".mesh",
        .import_func = ImportMesh
    };
}

