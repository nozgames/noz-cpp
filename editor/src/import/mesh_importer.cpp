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
    float dpi = (float)atlas->impl->dpi;

    // Convert pixel bounds to world units for quad vertices
    // pixel bounds are relative to where mesh_bounds.min was rendered
    Vec2 mesh_size = GetSize(rect.mesh_bounds);
    float expected_pixel_w = mesh_size.x * dpi;
    float expected_pixel_h = mesh_size.y * dpi;

    // Calculate how much the actual content extends beyond expected bounds (in world units)
    float offset_min_x = (rect.pixel_min_x - ATLAS_RECT_PADDING) / dpi;
    float offset_min_y = (rect.pixel_min_y - ATLAS_RECT_PADDING) / dpi;
    float offset_max_x = (rect.pixel_max_x - ATLAS_RECT_PADDING - expected_pixel_w + 1) / dpi;
    float offset_max_y = (rect.pixel_max_y - ATLAS_RECT_PADDING - expected_pixel_h + 1) / dpi;

    Vec2 min = rect.mesh_bounds.min + Vec2{offset_min_x, offset_min_y};
    Vec2 max = rect.mesh_bounds.max + Vec2{offset_max_x, offset_max_y};

    // UVs map to texel centers to ensure point filtering samples valid pixels
    // Using pixel edges (pixel_max + 1) can sample outside content at boundaries
    float u_min = (float)(rect.x + rect.pixel_min_x + 0.5f) / (float)atlas->impl->width;
    float v_min = (float)(rect.y + rect.pixel_min_y + 0.5f) / (float)atlas->impl->height;
    float u_max = (float)(rect.x + rect.pixel_max_x + 0.5f) / (float)atlas->impl->width;
    float v_max = (float)(rect.y + rect.pixel_max_y + 0.5f) / (float)atlas->impl->height;

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

// Check if mesh is skinned (has a skeleton or any bone weights) AND has valid geometry
static bool IsSkinnedMesh(MeshData* mesh) {
    if (!mesh || !mesh->impl) return false;
    if (mesh->impl->frame_count == 0) return false;

    // Must have face data to be triangulated
    MeshFrameData* frame = &mesh->impl->frames[0];
    if (frame->face_count == 0 || frame->vertex_count == 0) return false;

    // Check if mesh has a skeleton reference
    if (mesh->impl->skeleton_name != nullptr) return true;

    // Also check if any vertex has non-zero bone weights
    for (int v = 0; v < frame->vertex_count; v++) {
        const VertexData& vertex = frame->vertices[v];
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            if (vertex.weights[w].weight > F32_EPSILON) {
                return true;
            }
        }
    }
    return false;
}

// Create triangulated mesh with atlas UVs (for skinned meshes that need bone weights preserved)
static Mesh* ToMeshWithAtlasUVsTriangulated(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect) {
    // Ensure mesh data has faces before trying to triangulate
    if (!mesh_data || !mesh_data->impl || mesh_data->impl->frames[0].face_count == 0) {
        return nullptr;
    }

    // Get the triangulated mesh with bone weights
    Mesh* tri_mesh = ToMesh(mesh_data, false, false);
    if (!tri_mesh) return nullptr;

    // Create new mesh builder to rebuild with atlas UVs
    u16 vertex_count = GetVertexCount(tri_mesh);
    u16 index_count = GetIndexCount(tri_mesh);

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, vertex_count, index_count);

    const MeshVertex* src_vertices = GetVertices(tri_mesh);
    const u16* src_indices = GetIndices(tri_mesh);
    int atlas_idx = GetAtlasIndex(atlas);

    // Remap each vertex's UV to atlas UV, preserve bone data
    for (int i = 0; i < vertex_count; i++) {
        MeshVertex v = src_vertices[i];
        // Replace UV with atlas UV based on vertex position
        // Use bounds stored in rect to match what was rendered to atlas
        v.uv = GetAtlasUV(atlas, rect, rect.mesh_bounds, v.position);
        v.atlas_index = atlas_idx;
        // bone_indices and bone_weights are already set in v from src_vertices
        AddVertex(builder, v);
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
        // Skinned meshes need triangulated geometry to preserve bone weights
        if (IsSkinnedMesh(mesh_data)) {
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

