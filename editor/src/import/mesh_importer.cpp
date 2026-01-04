//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

namespace fs = std::filesystem;
using namespace noz;

constexpr Vec2 OUTLINE_COLOR = ColorUV(0, 10);

struct OutlineConfig {
    float width;
    float offset;
    float boundary_taper;
};

static Mesh* ToMeshWithAtlasUVs(MeshData* mesh_data, AtlasData* atlas, const AtlasRect& rect) {
    // Create a mesh with UVs mapping into the atlas texture instead of palette colors
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_VERTICES, MAX_INDICES);

    float depth = 0.01f + 0.99f * (mesh_data->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);

    // Process each face
    for (int fi = 0; fi < mesh_data->impl->face_count; fi++) {
        FaceData* f = &mesh_data->impl->faces[fi];
        if (f->vertex_count < 3) continue;

        SetBaseVertex(builder, GetVertexCount(builder));

        // Add vertices with atlas UVs
        for (int vi = 0; vi < f->vertex_count; vi++) {
            VertexData& v = mesh_data->impl->vertices[f->vertices[vi]];

            // Compute UV based on position in atlas rect
            Vec2 uv = GetAtlasUV(atlas, rect, v.position);

            MeshVertex mv = {
                .position = v.position,
                .depth = depth,
                .uv = uv
            };
            mv.bone_weights.x = v.weights[0].weight;
            mv.bone_weights.y = v.weights[1].weight;
            mv.bone_weights.z = v.weights[2].weight;
            mv.bone_weights.w = v.weights[3].weight;
            mv.bone_indices.x = v.weights[0].bone_index;
            mv.bone_indices.y = v.weights[1].bone_index;
            mv.bone_indices.z = v.weights[2].bone_index;
            mv.bone_indices.w = v.weights[3].bone_index;
            AddVertex(builder, mv);
        }

        // Triangulate using ear clipping
        for (u16 i = 1; i < f->vertex_count - 1; i++) {
            AddIndex(builder, 0);
            AddIndex(builder, i);
            AddIndex(builder, (u16)(i + 1));
        }
    }

    Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, mesh_data->name, false);
    PopScratch();
    return mesh;
}

static void ImportMesh(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* mesh_data = static_cast<MeshData*>(a);

    Mesh* m = nullptr;

    // Check if mesh is attached to an atlas
    if (mesh_data->impl->atlas_name) {
        AssetData* atlas_asset = GetAssetData(ASSET_TYPE_ATLAS, mesh_data->impl->atlas_name);
        if (atlas_asset) {
            AtlasData* atlas = static_cast<AtlasData*>(atlas_asset);

            // Find or create rect for this mesh
            AtlasRect* rect = FindRectForMesh(atlas, mesh_data->name);
            if (!rect) {
                rect = AllocateRect(atlas, mesh_data->name, mesh_data->bounds);
                if (rect) {
                    RenderMeshToAtlas(atlas, mesh_data, *rect);
                }
            }

            if (rect) {
                m = ToMeshWithAtlasUVs(mesh_data, atlas, *rect);
            }
        }
    }

    // Fall back to normal mesh if no atlas
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

