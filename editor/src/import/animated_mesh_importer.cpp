//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

static Mesh* ToFrameWithAtlasUVs(MeshData* frame_mesh, AtlasData* atlas, const AtlasRect& rect,
                                  int frame_idx, const Bounds2& max_bounds) {
    // Create a simple quad with UVs mapping into the atlas texture for this frame
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 4, 6);
    AtlasDataImpl* atlas_impl = atlas->impl;

    float depth = 0.01f + 0.99f * (frame_mesh->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);

    // Use max_bounds for consistent geometry across all frames
    Vec2 min = max_bounds.min;
    Vec2 max = max_bounds.max;

    // Calculate frame dimensions within the strip
    int frame_width = rect.width / rect.frame_count;
    float frame_x = (float)(rect.x + frame_idx * frame_width);

    // Inner rect with 1px padding
    float inner_x = frame_x + 1.0f;
    float inner_y = rect.y + 1.0f;
    float inner_w = frame_width - 2.0f;
    float inner_h = rect.height - 2.0f;

    // Half-texel inset to prevent bilinear bleed
    float half_texel_u = 0.5f / (float)atlas_impl->width;
    float half_texel_v = 0.5f / (float)atlas_impl->height;

    // Calculate UV bounds for this frame
    float min_u = (inner_x / (float)atlas_impl->width) + half_texel_u;
    float min_v = (inner_y / (float)atlas_impl->height) + half_texel_v;
    float max_u = ((inner_x + inner_w) / (float)atlas_impl->width) - half_texel_u;
    float max_v = ((inner_y + inner_h) / (float)atlas_impl->height) - half_texel_v;

    // Four corners of the mesh bounds with atlas UVs
    Vec2 uv_bl = {min_u, min_v};
    Vec2 uv_br = {max_u, min_v};
    Vec2 uv_tr = {max_u, max_v};
    Vec2 uv_tl = {min_u, max_v};

    AddVertex(builder, {{min.x, min.y}, depth, uv_bl});
    AddVertex(builder, {{max.x, min.y}, depth, uv_br});
    AddVertex(builder, {{max.x, max.y}, depth, uv_tr});
    AddVertex(builder, {{min.x, max.y}, depth, uv_tl});

    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);

    Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, frame_mesh->name, false);
    Free(builder);
    return mesh;
}

static void ImportAnimatedMesh(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_ANIMATED_MESH;
    header.version = 1;

    // Check if animated mesh is in an atlas
    AtlasRect* rect = nullptr;
    AtlasData* atlas = FindAtlasForMesh(m->name, &rect);

    // Calculate max bounds across all frames (same as in atlas allocation)
    Bounds2 max_bounds = m->impl->frames[0].bounds;
    for (int i = 1; i < m->impl->frame_count; i++) {
        max_bounds = Union(max_bounds, m->impl->frames[i].bounds);
    }

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    WriteAssetHeader(stream, &header);
    WriteStruct(stream, max_bounds);  // Use max_bounds for consistent sizing
    WriteU8(stream, (u8)ANIMATION_FRAME_RATE);
    WriteU8(stream, (u8)m->impl->frame_count);
    for (int i = 0; i < m->impl->frame_count; i++) {
        MeshData* frame = &m->impl->frames[i];
        Mesh* frame_mesh = nullptr;

        if (atlas && rect) {
            // Export as simple quad with atlas UVs
            frame_mesh = ToFrameWithAtlasUVs(frame, atlas, *rect, i, max_bounds);
        } else {
            // Fall back to normal mesh
            frame_mesh = ToMesh(frame, false);
        }

        SerializeMesh(frame_mesh, stream);
    }

    SaveStream(stream, path);
    Free(stream);
}

AssetImporter GetAnimatedMeshImporter() {
    return {
        .type = ASSET_TYPE_ANIMATED_MESH,
        .ext = ".amesh",
        .import_func = ImportAnimatedMesh
    };
}

