//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <plutovg.h>
#include "utils/rect_packer.h"

using namespace noz;

extern void InitAtlasEditor(AtlasData* atlas);


static void DrawAtlasData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ATLAS);

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    DrawBounds(a, 0.05f);

    // Sync pixels to GPU if dirty
    SyncAtlasTexture(atlas);

    // Scale factor to display atlas in world units
    float scale = 10.0f / (float)impl->width;  // Normalize to 10 units
    Vec2 size = Vec2{(float)impl->width, (float)impl->height} * scale;

    // Draw the atlas texture if available
    if (impl->material) {
        BindDepth(-0.1f);
        BindColor(COLOR_WHITE);
        BindMaterial(impl->material);
        // Flip Y because texture coords are top-down
        DrawMesh(g_view.quad_mesh, Translate(a->position) * Scale(Vec2{size.x, -size.y}));
    } else {
        // Draw a gray placeholder
        BindDepth(0.0f);
        BindMaterial(g_view.editor_material);
        BindColor(Color{0.2f, 0.2f, 0.2f, 1.0f});
        DrawMesh(g_view.quad_mesh, Translate(a->position) * Scale(size));
    }

    // Draw rect outlines for each attached mesh
    BindDepth(0.1f);
    BindMaterial(g_view.editor_material);
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        Vec2 rect_pos = Vec2{(float)impl->rects[i].x, (float)impl->rects[i].y} * scale;
        Vec2 rect_size = Vec2{(float)impl->rects[i].width, (float)impl->rects[i].height} * scale;

        // Position relative to atlas origin (top-left)
        Vec2 center = a->position - size * 0.5f + rect_pos + rect_size * 0.5f;
        // Flip Y
        center.y = a->position.y + size.y * 0.5f - rect_pos.y - rect_size.y * 0.5f;

        BindColor(Color{0.4f, 0.8f, 0.4f, 0.3f});
        DrawMesh(g_view.quad_mesh, Translate(center) * Scale(rect_size));
    }
}

static void AllocAtlasDataImpl(AssetData* a) {
    assert(a->type == ASSET_TYPE_ATLAS);
    AtlasData* atlas = static_cast<AtlasData*>(a);
    atlas->impl = static_cast<AtlasDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(AtlasDataImpl)));
    memset(atlas->impl, 0, sizeof(AtlasDataImpl));
    atlas->impl->width = ATLAS_DEFAULT_SIZE;
    atlas->impl->height = ATLAS_DEFAULT_SIZE;
    atlas->impl->dpi = ATLAS_DEFAULT_DPI;
    atlas->impl->dirty = true;
    // packer is created in LoadAtlasData with correct dimensions
}

static void DestroyAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    if (atlas->impl) {
        if (atlas->impl->pixels) {
            Free(atlas->impl->pixels);
            atlas->impl->pixels = nullptr;
        }
        if (atlas->impl->packer) {
            delete atlas->impl->packer;
            atlas->impl->packer = nullptr;
        }
        Free(atlas->impl);
        atlas->impl = nullptr;
    }
}

static void CloneAtlasData(AssetData* a) {
    assert(a->type == ASSET_TYPE_ATLAS);
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* old_impl = atlas->impl;
    int old_width = old_impl->width;
    int old_height = old_impl->height;
    AllocAtlasDataImpl(a);
    memcpy(atlas->impl, old_impl, sizeof(AtlasDataImpl));
    // Don't share pixel buffer or packer - create new ones
    atlas->impl->pixels = nullptr;
    atlas->impl->texture = nullptr;
    atlas->impl->material = nullptr;
    atlas->impl->packer = new RectPacker(old_width, old_height);
    atlas->impl->dirty = true;

    // Mark existing rects as used in new packer
    for (int i = 0; i < atlas->impl->rect_count; i++) {
        if (atlas->impl->rects[i].valid) {
            const AtlasRect& r = atlas->impl->rects[i];
            RectPacker::Rect bin_rect(r.x, r.y, r.width, r.height);
            atlas->impl->packer->MarkUsed(bin_rect);
        }
    }
}

// Rect management

AtlasRect* FindRectForMesh(AtlasData* atlas, const Name* mesh_name) {
    AtlasDataImpl* impl = atlas->impl;
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name == mesh_name) {
            return &impl->rects[i];
        }
    }
    return nullptr;
}

static int FindFreeRectSlot(AtlasDataImpl* impl) {
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) return i;
    }
    if (impl->rect_count < ATLAS_MAX_RECTS) {
        return impl->rect_count++;
    }
    return -1;
}

AtlasRect* AllocateRect(AtlasData* atlas, MeshData* mesh) {
    AtlasDataImpl* impl = atlas->impl;

    // Calculate required size in pixels from mesh bounds
    Vec2 size = GetSize(mesh->bounds);
    int req_width = (int)(size.x * impl->dpi) + 2;  // +2 for padding
    int req_height = (int)(size.y * impl->dpi) + 2;

    if (req_width > impl->width || req_height > impl->height) {
        return nullptr;  // Too large for atlas
    }

    // Find a free slot
    int slot = FindFreeRectSlot(impl);
    if (slot < 0) return nullptr;

    // Use rect_packer to find position
    // Note: Use BottomLeftRule for Tetris-style packing (fills from top-left, no rotation)
    RectPacker::Rect bin_rect;
    int result = impl->packer->Insert(req_width, req_height, RectPacker::method::BottomLeftRule, bin_rect);
    if (result < 0) {
        return nullptr;  // No room (Insert returns -1 on failure, >= 0 on success)
    }

    AtlasRect& rect = impl->rects[slot];
    rect.x = bin_rect.x;
    rect.y = bin_rect.y;
    rect.width = bin_rect.w;
    rect.height = bin_rect.h;
    rect.mesh_name = mesh->name;
    rect.valid = true;

    impl->dirty = true;
    return &rect;
}

void FreeRect(AtlasData* atlas, AtlasRect* rect) {
    if (rect) {
        rect->valid = false;
        rect->mesh_name = nullptr;
        atlas->impl->dirty = true;
    }
}

void ClearAllRects(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    for (int i = 0; i < impl->rect_count; i++) {
        impl->rects[i].valid = false;
        impl->rects[i].mesh_name = nullptr;
    }
    impl->rect_count = 0;
    impl->dirty = true;

    // Reset packer
    if (impl->packer) {
        delete impl->packer;
    }
    impl->packer = new RectPacker(impl->width, impl->height);
}

// PlutoVG rendering

static void EnsurePixelBuffer(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels) {
        impl->pixels = (u8*)Alloc(ALLOCATOR_DEFAULT, impl->width * impl->height * 4);
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }
}

void RenderMeshToAtlas(AtlasData* atlas, MeshData* mesh, const AtlasRect& rect) {
    AtlasDataImpl* impl = atlas->impl;
    MeshDataImpl* mesh_impl = mesh->impl;

    EnsurePixelBuffer(atlas);

    // Create a surface for the entire atlas
    plutovg_surface_t* surface = plutovg_surface_create_for_data(
        impl->pixels, impl->width, impl->height, impl->width * 4
    );
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);

    // Clip to the rect
    plutovg_canvas_clip_rect(canvas, (float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height);

    // Set up transform: mesh coords -> pixel coords
    float scale = (float)impl->dpi;
    float offset_x = rect.x + 1 - mesh->bounds.min.x * scale;
    float offset_y = rect.y + 1 - mesh->bounds.min.y * scale;

    // Render each face
    for (int fi = 0; fi < mesh_impl->face_count; fi++) {
        FaceData& face = mesh_impl->faces[fi];
        if (face.vertex_count < 3) continue;

        plutovg_canvas_new_path(canvas);

        // Build path for this face
        for (int vi = 0; vi < face.vertex_count; vi++) {
            int v_idx = face.vertices[vi];
            Vec2 pos = mesh_impl->vertices[v_idx].position;

            // Transform to pixel coordinates
            float px = offset_x + pos.x * scale;
            float py = offset_y + pos.y * scale;

            if (vi == 0) {
                plutovg_canvas_move_to(canvas, px, py);
            } else {
                // Check if edge to this vertex is curved
                int prev_vi = (vi - 1 + face.vertex_count) % face.vertex_count;
                int prev_v_idx = face.vertices[prev_vi];

                // Find the edge between prev_v_idx and v_idx
                int edge_idx = GetEdge(mesh, prev_v_idx, v_idx);
                if (edge_idx >= 0 && IsEdgeCurved(mesh, edge_idx)) {
                    // Quadratic bezier curve
                    Vec2 control = GetEdgeControlPoint(mesh, edge_idx);
                    float cpx = offset_x + control.x * scale;
                    float cpy = offset_y + control.y * scale;
                    plutovg_canvas_quad_to(canvas, cpx, cpy, px, py);
                } else {
                    plutovg_canvas_line_to(canvas, px, py);
                }
            }
        }

        // Close the path - handle potential curve on last edge
        int last_v_idx = face.vertices[face.vertex_count - 1];
        int first_v_idx = face.vertices[0];
        int edge_idx = GetEdge(mesh, last_v_idx, first_v_idx);
        if (edge_idx >= 0 && IsEdgeCurved(mesh, edge_idx)) {
            Vec2 control = GetEdgeControlPoint(mesh, edge_idx);
            Vec2 first_pos = mesh_impl->vertices[first_v_idx].position;
            float cpx = offset_x + control.x * scale;
            float cpy = offset_y + control.y * scale;
            float px = offset_x + first_pos.x * scale;
            float py = offset_y + first_pos.y * scale;
            plutovg_canvas_quad_to(canvas, cpx, cpy, px, py);
        } else {
            plutovg_canvas_close_path(canvas);
        }

        // Get face color from palette
        int palette_index = g_editor.palette_map[mesh_impl->palette];
        Color color = g_editor.palettes[palette_index].colors[face.color];
        plutovg_canvas_set_rgba(canvas, color.r, color.g, color.b, color.a);
        plutovg_canvas_fill(canvas);
    }

    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);

    impl->dirty = true;
}

void SyncAtlasTexture(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels || !impl->dirty) return;

    // Convert from premultiplied ARGB to RGBA for GPU upload
    // We need a temporary buffer since we want to keep impl->pixels in ARGB for further PlutoVG rendering
    u32 buffer_size = impl->width * impl->height * 4;
    u8* rgba_pixels = static_cast<u8 *>(Alloc(ALLOCATOR_SCRATCH, buffer_size));
    memcpy(rgba_pixels, impl->pixels, buffer_size);
    plutovg_convert_argb_to_rgba(rgba_pixels, rgba_pixels, impl->width, impl->height, impl->width * 4);

    if (!impl->texture) {
        impl->texture = CreateTexture(ALLOCATOR_DEFAULT, rgba_pixels, impl->width, impl->height,
                                       TEXTURE_FORMAT_RGBA8, GetName("atlas_texture"));
        impl->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
        SetTexture(impl->material, impl->texture, 0);
    } else {
        UpdateTexture(impl->texture, rgba_pixels);
    }

    impl->dirty = false;
}

void RegenerateAtlas(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;

    // Clear pixel buffer
    if (impl->pixels) {
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }

    // Re-render all attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        // Load the mesh
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (mesh_asset) {
            MeshData* mesh = static_cast<MeshData*>(mesh_asset);
            RenderMeshToAtlas(atlas, mesh, impl->rects[i]);
        }
    }

    impl->dirty = true;
}

void RebuildAtlas(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;

    // Collect mesh names before clearing
    const Name* mesh_names[ATLAS_MAX_RECTS];
    int mesh_count = 0;
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            mesh_names[mesh_count++] = impl->rects[i].mesh_name;
        }
    }

    int original_count = mesh_count;

    // Clear all rects (resets packer)
    ClearAllRects(atlas);

    // Clear pixel buffer
    if (impl->pixels) {
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }

    // Re-allocate and render each mesh
    int success_count = 0;
    for (int i = 0; i < mesh_count; i++) {
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, mesh_names[i]);
        if (!mesh_asset) {
            LogError("RebuildAtlas: mesh '%s' not found", mesh_names[i]->value);
            continue;
        }

        MeshData* mesh = static_cast<MeshData*>(mesh_asset);

        // Allocate fresh rect
        AtlasRect* rect = AllocateRect(atlas, mesh);
        if (rect) {
            RenderMeshToAtlas(atlas, mesh, *rect);
            success_count++;
        } else {
            LogError("RebuildAtlas: failed to allocate rect for '%s'", mesh_names[i]->value);
        }

        // Mark mesh as modified
        MarkModified(mesh_asset);
    }

    if (success_count < original_count) {
        LogWarning("RebuildAtlas: only %d of %d meshes were successfully added", success_count, original_count);
    }

    impl->dirty = true;
}

// UV computation

Vec2 GetAtlasUV(AtlasData* atlas, const AtlasRect& rect, const Bounds2& mesh_bounds, const Vec2& position) {
    AtlasDataImpl* impl = atlas->impl;

    // Map position from mesh space to normalized [0,1] within mesh bounds
    float u = (position.x - mesh_bounds.min.x) / GetSize(mesh_bounds).x;
    float v = (position.y - mesh_bounds.min.y) / GetSize(mesh_bounds).y;

    // Inner region is 1 pixel inset from rect edges (padding from AllocateRect)
    // Add half-texel inset for bilinear filtering safety
    float inner_x = rect.x + 1.0f;
    float inner_y = rect.y + 1.0f;
    float inner_w = rect.width - 2.0f;
    float inner_h = rect.height - 2.0f;

    // Half-texel inset to prevent bilinear bleed
    float half_texel_u = 0.5f / (float)impl->width;
    float half_texel_v = 0.5f / (float)impl->height;

    // Map to inner rect with half-texel inset
    float min_u = (inner_x / (float)impl->width) + half_texel_u;
    float min_v = (inner_y / (float)impl->height) + half_texel_v;
    float max_u = ((inner_x + inner_w) / (float)impl->width) - half_texel_u;
    float max_v = ((inner_y + inner_h) / (float)impl->height) - half_texel_v;

    // Interpolate within the safe region
    u = min_u + u * (max_u - min_u);
    v = min_v + v * (max_v - min_v);

    return {u, v};
}

// Save/Load

static void SaveAtlasData(AssetData* a, const std::filesystem::path& path) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

    WriteCSTR(stream, "w %d\n", impl->width);
    WriteCSTR(stream, "h %d\n", impl->height);
    WriteCSTR(stream, "d %d\n", impl->dpi);
    WriteCSTR(stream, "\n");

    // Save rects: r "mesh_name" x y w h
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            const AtlasRect& r = impl->rects[i];
            WriteCSTR(stream, "r \"%s\" %d %d %d %d\n",
                r.mesh_name->value,
                r.x, r.y, r.width, r.height);
        }
    }

    SaveStream(stream, path);
    Free(stream);
}

static void ParseRect(AtlasData* atlas, Tokenizer& tk) {
    AtlasDataImpl* impl = atlas->impl;

    if (!ExpectQuotedString(tk))
        ThrowError("missing rect mesh name");

    const Name* mesh_name = GetName(tk);

    int x, y, w, h;
    if (!ExpectInt(tk, &x)) ThrowError("missing rect x");
    if (!ExpectInt(tk, &y)) ThrowError("missing rect y");
    if (!ExpectInt(tk, &w)) ThrowError("missing rect width");
    if (!ExpectInt(tk, &h)) ThrowError("missing rect height");

    int slot = FindFreeRectSlot(impl);
    if (slot >= 0) {
        AtlasRect& rect = impl->rects[slot];
        rect.mesh_name = mesh_name;
        rect.x = x;
        rect.y = y;
        rect.width = w;
        rect.height = h;
        rect.valid = true;

        // Mark this space as used in the packer
        RectPacker::Rect bin_rect(x, y, w, h);
        impl->packer->MarkUsed(bin_rect);
    }
}

static void LoadAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path.value);
    Tokenizer tk;
    Init(tk, contents.c_str());

    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "w")) {
            if (!ExpectInt(tk, &impl->width))
                ThrowError("missing atlas width");
        } else if (ExpectIdentifier(tk, "h")) {
            if (!ExpectInt(tk, &impl->height))
                ThrowError("missing atlas height");
        } else if (ExpectIdentifier(tk, "d")) {
            if (!ExpectInt(tk, &impl->dpi))
                ThrowError("missing atlas dpi");
        } else if (ExpectIdentifier(tk, "r")) {
            // Ensure packer is initialized with correct dimensions before parsing rects
            if (!impl->packer) {
                impl->packer = new RectPacker(impl->width, impl->height);
            }
            ParseRect(atlas, tk);
        } else {
            char error[256];
            GetString(tk, error, sizeof(error) - 1);
            ThrowError("invalid token '%s' in atlas", error);
        }
    }

    // Initialize packer if no rects were loaded
    if (!impl->packer) {
        impl->packer = new RectPacker(impl->width, impl->height);
    }

    // Use same scale as DrawAtlasData (10 units for full width)
    float scale = 10.0f / (float)impl->width;
    Vec2 tsize = Vec2{static_cast<float>(impl->width), static_cast<float>(impl->height)} * scale;
    a->bounds = Bounds2{-tsize.x*0.5f, -tsize.y*0.5f, tsize.x*0.5f, tsize.y*0.5f};
}

static void PostLoadAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Render all valid rects now that meshes are loaded
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                RenderMeshToAtlas(atlas, mesh, impl->rects[i]);
            }
        }
    }
}

static void LoadAtlasMetaData(AssetData* a, Props* meta) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    impl->width = meta->GetInt("atlas", "width", ATLAS_DEFAULT_SIZE);
    impl->height = meta->GetInt("atlas", "height", ATLAS_DEFAULT_SIZE);
    impl->dpi = meta->GetInt("atlas", "dpi", ATLAS_DEFAULT_DPI);

    // Resize packer to match loaded dimensions
    if (impl->packer) {
        impl->packer->Resize(impl->width, impl->height);
    }

    InitAtlasEditor(atlas);
}

static void SaveAtlasMetaData(AssetData* a, Props* meta) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    meta->SetInt("atlas", "width", impl->width);
    meta->SetInt("atlas", "height", impl->height);
    meta->SetInt("atlas", "dpi", impl->dpi);
}

void InitAtlasData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ATLAS);

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AllocAtlasDataImpl(a);

    atlas->bounds = Bounds2{Vec2{-5.0f, -5.0f}, Vec2{5.0f, 5.0f}};
    atlas->vtable = {
        .destructor = DestroyAtlasData,
        .load = LoadAtlasData,
        .post_load = PostLoadAtlasData,
        .save = SaveAtlasData,
        .load_metadata = LoadAtlasMetaData,
        .save_metadata = SaveAtlasMetaData,
        .draw = DrawAtlasData,
        .clone = CloneAtlasData,
    };
}

AssetData* NewAtlasData(const std::filesystem::path& path) {
    constexpr const char* default_atlas = "w 1024\nh 1024\nd 96\n";

    std::string type_folder = ToString(ASSET_TYPE_ATLAS);
    Lower(type_folder.data(), (u32)type_folder.size());

    std::filesystem::path full_path = path.is_relative()
        ? std::filesystem::path(g_editor.project_path) / g_editor.save_dir / type_folder / path
        : path;
    full_path += ".atlas";

    // Create directory if it doesn't exist
    std::filesystem::create_directories(full_path.parent_path());

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 256);
    WriteCSTR(stream, default_atlas);
    SaveStream(stream, full_path);
    Free(stream);

    AtlasData* atlas = static_cast<AtlasData*>(CreateAssetData(full_path));
    LoadAtlasData(atlas);
    return atlas;
}
