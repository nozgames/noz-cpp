//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <plutovg.h>
#include "utils/rect_packer.h"
#include "atlas_manager.h"

using namespace noz;

extern void InitAtlasEditor(AtlasData* atlas);

// Calculate bounds from vertices in a frame
static Bounds2 GetFrameBounds(MeshFrameData* frame) {
    if (frame->vertex_count == 0)
        return BOUNDS2_ZERO;

    Bounds2 bounds = { frame->vertices[0].position, frame->vertices[0].position };
    for (int i = 1; i < frame->vertex_count; i++) {
        bounds.min.x = Min(bounds.min.x, frame->vertices[i].position.x);
        bounds.min.y = Min(bounds.min.y, frame->vertices[i].position.y);
        bounds.max.x = Max(bounds.max.x, frame->vertices[i].position.x);
        bounds.max.y = Max(bounds.max.y, frame->vertices[i].position.y);
    }
    return bounds;
}


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

    // Unregister from atlas manager
    UnregisterManagedAtlas(atlas);

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

AtlasData* FindAtlasForMesh(const Name* mesh_name, AtlasRect** out_rect) {
    // Search all atlas assets for one containing this mesh
    for (int i = 0, count = GetAssetCount(); i < count; i++) {
        AssetData* asset = GetAssetData(i);
        if (asset->type != ASSET_TYPE_ATLAS) continue;

        AtlasData* atlas = static_cast<AtlasData*>(asset);
        AtlasRect* rect = FindRectForMesh(atlas, mesh_name);
        if (rect) {
            if (out_rect) *out_rect = rect;
            return atlas;
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
    MeshDataImpl* mesh_impl = mesh->impl;

    if (mesh_impl->frame_count == 0) return nullptr;

    // Calculate max bounds across all frames
    Bounds2 max_bounds = GetFrameBounds(&mesh_impl->frames[0]);
    for (int i = 1; i < mesh_impl->frame_count; i++) {
        max_bounds = Union(max_bounds, GetFrameBounds(&mesh_impl->frames[i]));
    }

    Vec2 frame_size = GetSize(max_bounds);
    int frame_width = (int)(frame_size.x * impl->dpi) + 2;   // +2 for padding
    int frame_height = (int)(frame_size.y * impl->dpi) + 2;

    // Total strip size: frames laid out horizontally
    int total_width = frame_width * mesh_impl->frame_count;
    int total_height = frame_height;

    if (total_width > impl->width || total_height > impl->height) {
        return nullptr;  // Too large for atlas
    }

    int slot = FindFreeRectSlot(impl);
    if (slot < 0) return nullptr;

    RectPacker::Rect bin_rect;
    int result = impl->packer->Insert(total_width, total_height, RectPacker::method::BottomLeftRule, bin_rect);
    if (result < 0) {
        return nullptr;
    }

    AtlasRect& rect = impl->rects[slot];
    rect.x = bin_rect.x;
    rect.y = bin_rect.y;
    rect.width = bin_rect.w;
    rect.height = bin_rect.h;
    rect.mesh_name = mesh->name;
    rect.valid = true;
    rect.frame_count = mesh_impl->frame_count;

    impl->dirty = true;
    return &rect;
}

void FreeRect(AtlasData* atlas, AtlasRect* rect) {
    if (rect) {
        // Clear the mesh's atlas reference
        if (rect->mesh_name) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, rect->mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                mesh->impl->atlas = nullptr;
            }
        }
        rect->valid = false;
        rect->mesh_name = nullptr;
        rect->frame_count = 1;
        atlas->impl->dirty = true;
    }
}

void ClearAllRects(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    for (int i = 0; i < impl->rect_count; i++) {
        // Clear the mesh's atlas reference
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                mesh->impl->atlas = nullptr;
            }
        }
        impl->rects[i].valid = false;
        impl->rects[i].mesh_name = nullptr;
        impl->rects[i].frame_count = 1;
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

// Frame-based version for multi-frame rendering
static void AddFaceToPathFrame(plutovg_canvas_t* canvas, MeshFrameData* frame, FaceData& face, float offset_x, float offset_y, float scale, float expand = 0.0f) {
    // Calculate face center for expansion direction
    Vec2 center = {0, 0};
    for (int vi = 0; vi < face.vertex_count; vi++) {
        center = center + frame->vertices[face.vertices[vi]].position;
    }
    center = center / (float)face.vertex_count;

    for (int vi = 0; vi < face.vertex_count; vi++) {
        int v_idx = face.vertices[vi];
        Vec2 pos = frame->vertices[v_idx].position;

        // Expand vertex outward from face center
        if (expand > 0.0f) {
            Vec2 dir = Normalize(pos - center);
            pos = pos + dir * (expand / scale);
        }

        float px = offset_x + pos.x * scale;
        float py = offset_y + pos.y * scale;

        if (vi == 0) {
            plutovg_canvas_move_to(canvas, px, py);
        } else {
            // For multi-frame meshes, we skip curve handling (edges are frame-specific)
            // Edges with curves should use the MeshData version
            plutovg_canvas_line_to(canvas, px, py);
        }
    }

    plutovg_canvas_close_path(canvas);
}

void RenderMeshToAtlas(AtlasData* atlas, MeshData* mesh, const AtlasRect& rect) {
    AtlasDataImpl* impl = atlas->impl;
    MeshDataImpl* mesh_impl = mesh->impl;

    if (mesh_impl->frame_count == 0) return;

    EnsurePixelBuffer(atlas);

    // Calculate max bounds across all frames
    Bounds2 max_bounds = GetFrameBounds(&mesh_impl->frames[0]);
    for (int i = 1; i < mesh_impl->frame_count; i++) {
        max_bounds = Union(max_bounds, GetFrameBounds(&mesh_impl->frames[i]));
    }

    float scale = (float)impl->dpi;
    int frame_width = rect.width / rect.frame_count;

    // Render each frame at its position in the strip
    for (int frame_idx = 0; frame_idx < mesh_impl->frame_count; frame_idx++) {
        MeshFrameData* frame = &mesh_impl->frames[frame_idx];

        int frame_x = rect.x + frame_idx * frame_width;

        plutovg_surface_t* surface = plutovg_surface_create_for_data(
            impl->pixels, impl->width, impl->height, impl->width * 4
        );
        plutovg_canvas_t* canvas = plutovg_canvas_create(surface);

        // Clip to this frame's region
        plutovg_canvas_clip_rect(canvas, (float)frame_x, (float)rect.y, (float)frame_width, (float)rect.height);

        // Use max_bounds for consistent positioning across all frames
        float offset_x = frame_x + 1 - max_bounds.min.x * scale;
        float offset_y = rect.y + 1 - max_bounds.min.y * scale;

        int palette_index = g_editor.palette_map[mesh_impl->palette];
        constexpr float EXPAND = 0.5f;

        // Render each color group
        bool rendered[MAX_FACES] = {};
        for (int fi = 0; fi < frame->face_count; fi++) {
            if (rendered[fi]) continue;

            FaceData& face = frame->faces[fi];
            if (face.vertex_count < 3) continue;

            int color_index = face.color;
            Color color = g_editor.palettes[palette_index].colors[color_index];

            plutovg_canvas_new_path(canvas);
            AddFaceToPathFrame(canvas, frame, face, offset_x, offset_y, scale, EXPAND);
            rendered[fi] = true;

            for (int fj = fi + 1; fj < frame->face_count; fj++) {
                if (rendered[fj]) continue;
                FaceData& other_face = frame->faces[fj];
                if (other_face.vertex_count < 3) continue;
                if (other_face.color != color_index) continue;

                AddFaceToPathFrame(canvas, frame, other_face, offset_x, offset_y, scale, EXPAND);
                rendered[fj] = true;
            }

            plutovg_canvas_set_rgba(canvas, color.r, color.g, color.b, color.a);
            plutovg_canvas_fill(canvas);
        }

        plutovg_canvas_destroy(canvas);
        plutovg_surface_destroy(surface);

        // Dilate content into padding to prevent bilinear filtering seams
        // Content is at (frame_x+1, rect.y+1) with size (frame_width-2, rect.height-2)
        int content_x = frame_x + 1;
        int content_y = rect.y + 1;
        int content_w = frame_width - 2;
        int content_h = rect.height - 2;
        int stride = impl->width * 4;
        u8* pixels = impl->pixels;

        // Top edge: copy row content_y to row content_y-1
        memcpy(pixels + (content_y - 1) * stride + content_x * 4,
               pixels + content_y * stride + content_x * 4,
               content_w * 4);

        // Bottom edge: copy row content_y+content_h-1 to row content_y+content_h
        memcpy(pixels + (content_y + content_h) * stride + content_x * 4,
               pixels + (content_y + content_h - 1) * stride + content_x * 4,
               content_w * 4);

        // Left edge: copy column content_x to column content_x-1
        for (int y = content_y; y < content_y + content_h; y++) {
            memcpy(pixels + y * stride + (content_x - 1) * 4,
                   pixels + y * stride + content_x * 4, 4);
        }

        // Right edge: copy column content_x+content_w-1 to column content_x+content_w
        for (int y = content_y; y < content_y + content_h; y++) {
            memcpy(pixels + y * stride + (content_x + content_w) * 4,
                   pixels + y * stride + (content_x + content_w - 1) * 4, 4);
        }

        // Corners
        memcpy(pixels + (content_y - 1) * stride + (content_x - 1) * 4,
               pixels + content_y * stride + content_x * 4, 4);  // Top-left
        memcpy(pixels + (content_y - 1) * stride + (content_x + content_w) * 4,
               pixels + content_y * stride + (content_x + content_w - 1) * 4, 4);  // Top-right
        memcpy(pixels + (content_y + content_h) * stride + (content_x - 1) * 4,
               pixels + (content_y + content_h - 1) * stride + content_x * 4, 4);  // Bottom-left
        memcpy(pixels + (content_y + content_h) * stride + (content_x + content_w) * 4,
               pixels + (content_y + content_h - 1) * stride + (content_x + content_w - 1) * 4, 4);  // Bottom-right
    }

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

    // Collect mesh names and frame counts before clearing
    struct RectInfo {
        const Name* name;
        int frame_count;
    };
    RectInfo rect_infos[ATLAS_MAX_RECTS];
    int rect_info_count = 0;
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            rect_infos[rect_info_count].name = impl->rects[i].mesh_name;
            rect_infos[rect_info_count].frame_count = impl->rects[i].frame_count;
            rect_info_count++;
        }
    }

    int original_count = rect_info_count;

    // Clear all rects (resets packer)
    ClearAllRects(atlas);

    // Clear pixel buffer
    if (impl->pixels) {
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }

    // Re-allocate and render each mesh
    int success_count = 0;
    for (int i = 0; i < rect_info_count; i++) {
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, rect_infos[i].name);
        if (!mesh_asset) {
            LogError("RebuildAtlas: mesh '%s' not found", rect_infos[i].name->value);
            continue;
        }

        MeshData* mesh = static_cast<MeshData*>(mesh_asset);
        AtlasRect* rect = AllocateRect(atlas, mesh);
        if (rect) {
            RenderMeshToAtlas(atlas, mesh, *rect);
            success_count++;
        } else {
            LogError("RebuildAtlas: failed to allocate rect for '%s'", rect_infos[i].name->value);
        }
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

    // For animated meshes, use only the first frame's width
    int frame_count = rect.frame_count > 0 ? rect.frame_count : 1;
    float frame_width = (float)rect.width / (float)frame_count;

    // Inner region is 1 pixel inset from rect edges (padding from AllocateRect)
    // Add half-texel inset for bilinear filtering safety
    float inner_x = rect.x + 1.0f;
    float inner_y = rect.y + 1.0f;
    float inner_w = frame_width - 2.0f;
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

    // Save rects: r "mesh_name" x y w h [frame_count]
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            const AtlasRect& r = impl->rects[i];
            if (r.frame_count > 1) {
                // Animated mesh with multiple frames
                WriteCSTR(stream, "r \"%s\" %d %d %d %d %d\n",
                    r.mesh_name->value,
                    r.x, r.y, r.width, r.height, r.frame_count);
            } else {
                // Static mesh (default frame_count = 1)
                WriteCSTR(stream, "r \"%s\" %d %d %d %d\n",
                    r.mesh_name->value,
                    r.x, r.y, r.width, r.height);
            }
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

    // Optional frame_count for animated meshes (default = 1)
    int frame_count = 1;
    ExpectInt(tk, &frame_count);

    int slot = FindFreeRectSlot(impl);
    if (slot >= 0) {
        AtlasRect& rect = impl->rects[slot];
        rect.mesh_name = mesh_name;
        rect.x = x;
        rect.y = y;
        rect.width = w;
        rect.height = h;
        rect.frame_count = frame_count;
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

    // Register as managed atlas if it has the auto-managed naming convention
    RegisterManagedAtlas(atlas);

    // Bind meshes to this atlas and render them
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                mesh->impl->atlas = atlas;
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
