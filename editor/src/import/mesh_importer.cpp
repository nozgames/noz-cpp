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

// Create mesh from pixel hull - works for all mesh types
static Mesh* ToMeshWithPixelHull(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect) {
    if (!mesh_data || !mesh_data->impl) return nullptr;

    // Ensure atlas has pixels rendered
    if (!atlas->impl->pixels) {
        RegenerateAtlas(atlas);
    }

    // Compute hull from rendered pixels (with small expand for bilinear filtering)
    Vec2 hull[256];
    float expand = ATLAS_HULL_EXPAND;  // In pixels
    int hull_count = ComputePixelHull(atlas, rect, hull, 256, expand);
    if (hull_count < 3) return nullptr;

    // Triangulate hull as fan from first vertex
    int tri_count = hull_count - 2;
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, (u16)hull_count, (u16)(tri_count * 3));

    float depth = 0.01f + 0.99f * (mesh_data->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);
    int atlas_idx = GetAtlasIndex(atlas);

    // Get bone index if skinned (single bone only)
    int bone_index = GetSingleBoneIndex(mesh_data);

    // Add hull vertices with atlas UVs
    for (int i = 0; i < hull_count; i++) {
        MeshVertex v = {};
        v.position = hull[i];
        v.depth = depth;
        v.opacity = 1.0f;
        v.uv = GetAtlasUV(atlas, rect, rect.mesh_bounds, hull[i]);
        v.atlas_index = atlas_idx;

        if (bone_index >= 0) {
            v.bone_indices = {bone_index, 0, 0, 0};
            v.bone_weights = {1.0f, 0.0f, 0.0f, 0.0f};
        }

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

    Mesh* m = nullptr;

    // Check if mesh is in an atlas - use pixel hull for all atlas meshes
    AtlasRect* rect = nullptr;
    AtlasData* atlas = FindAtlasForMesh(mesh_data->name, &rect);
    if (atlas && rect) {
        m = ToMeshWithPixelHull(mesh_data, atlas, *rect);
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

AssetImporter GetMeshImporter() {
    return {
        .type = ASSET_TYPE_MESH,
        .ext = ".mesh",
        .import_func = ImportMesh
    };
}

