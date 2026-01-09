//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "utils/rasterizer.h"
#include "utils/pixel_data.h"
#include "utils/rect_packer.h"
#include "atlas_manager.h"

using namespace noz;

extern void InitAtlasEditor(AtlasData* atlas);

// Calculate bounds from vertices and curve control points in a frame
static Bounds2 GetFrameBounds(MeshFrameData* frame) {
    if (frame->geom.vert_count == 0)
        return BOUNDS2_ZERO;

    Bounds2 bounds = { GetVertex(frame, 0)->position, GetVertex(frame, 0)->position };
    for (u16 vi = 1; vi < frame->geom.vert_count; vi++) {
        VertexData* v = GetVertex(frame, vi);
        bounds.min.x = Min(bounds.min.x, v->position.x);
        bounds.min.y = Min(bounds.min.y, v->position.y);
        bounds.max.x = Max(bounds.max.x, v->position.x);
        bounds.max.y = Max(bounds.max.y, v->position.y);
    }

    // Include curve control points in bounds
    for (u16 ei = 0; ei < frame->geom.edge_count; ei++) {
        EdgeData* e = GetEdge(frame, ei);
        if (LengthSqr(e->curve.offset) > FLT_EPSILON) {
            const Vec2& p0 = GetVertex(frame, e->v0)->position;
            const Vec2& p1 = GetVertex(frame, e->v1)->position;
            Vec2 control = (p0 + p1) * 0.5f + e->curve.offset;
            bounds.min.x = Min(bounds.min.x, control.x);
            bounds.min.y = Min(bounds.min.y, control.y);
            bounds.max.x = Max(bounds.max.x, control.x);
            bounds.max.y = Max(bounds.max.y, control.y);
        }
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

    // Scale factor to display atlas in world units (512 pixels = 10 units for grid alignment)
    constexpr float PIXELS_PER_UNIT = 51.2f;
    Vec2 size = ToVec2(impl->size) / PIXELS_PER_UNIT;

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
    atlas->impl->size.x = ATLAS_DEFAULT_SIZE;
    atlas->impl->size.y = ATLAS_DEFAULT_SIZE;
    atlas->impl->dpi = g_editor.atlas.dpi;
    atlas->impl->dirty = true;
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
        if (atlas->impl->outline_mesh) {
            Free(atlas->impl->outline_mesh);
            atlas->impl->outline_mesh = nullptr;
        }
        Free(atlas->impl);
        atlas->impl = nullptr;
    }
}

static void CloneAtlasData(AssetData* a) {
    assert(a->type == ASSET_TYPE_ATLAS);
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* old_impl = atlas->impl;
    int old_width = old_impl->size.x;
    int old_height = old_impl->size.y;
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
    int frame_width = (int)(frame_size.x * impl->dpi) + g_editor.atlas.padding * 2;
    int frame_height = (int)(frame_size.y * impl->dpi) + g_editor.atlas.padding * 2;

    // Total strip size: frames laid out horizontally
    int total_width = frame_width * mesh_impl->frame_count;
    int total_height = frame_height;

    if (total_width > impl->size.x || total_height > impl->size.y) {
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
    rect.mesh_bounds = max_bounds;  // Store bounds for UV calculation

    impl->dirty = true;
    impl->outline_dirty = true;
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
        atlas->impl->outline_dirty = true;
    }
}

void ClearRectPixels(AtlasData* atlas, const AtlasRect& rect) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels) return;

    // Clear each row of the rect to transparent black
    int stride = impl->size.x * 4;
    for (int y = rect.y; y < rect.y + rect.height && y < impl->size.y; y++) {
        int offset = y * stride + rect.x * 4;
        int bytes = rect.width * 4;
        if (rect.x + rect.width > impl->size.x) {
            bytes = (impl->size.x - rect.x) * 4;
        }
        memset(impl->pixels + offset, 0, bytes);
    }
    impl->dirty = true;
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
    impl->outline_dirty = true;

    // Reset packer
    if (impl->packer) {
        delete impl->packer;
    }
    impl->packer = new RectPacker(impl->size.x, impl->size.y);
}

static void EnsurePixelBuffer(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels)
        impl->pixels = CreatePixelData(ALLOCATOR_DEFAULT, impl->size);
}

void AddFaceToPath(
    Rasterizer* rasterizer,
    MeshFrameData* frame,
    FaceData& face,
    float offset_x,
    float offset_y);

void RenderMeshToBuffer(AtlasData* atlas, MeshData* mesh, AtlasRect& rect, PixelData* pixels, bool update_bounds) {
#if 0
    AtlasDataImpl* impl = atlas->impl;
    MeshDataImpl* mesh_impl = mesh->impl;

    if (mesh_impl->frame_count == 0) return;

    Bounds2 max_bounds = GetFrameBounds(&mesh_impl->frames[0]);
    for (int i = 1; i < mesh_impl->frame_count; i++)
        max_bounds = Union(max_bounds, GetFrameBounds(&mesh_impl->frames[i]));

    Bounds2 render_bounds;
    if (update_bounds) {
        rect.mesh_bounds = max_bounds;
        render_bounds = max_bounds;
    } else {
        render_bounds = rect.mesh_bounds;
    }

    float scale = (float)impl->dpi;
    int frame_width = rect.width / rect.frame_count;

    Rasterizer* rasterizer = CreateRasterizer(ALLOCATOR_DEFAULT, pixels);

    for (int frame_idx = 0; frame_idx < mesh_impl->frame_count; frame_idx++) {
        MeshFrameData* frame = &mesh_impl->frames[frame_idx];
        int frame_x = rect.x + frame_idx * frame_width;

        SetClipRect(rasterizer, frame_x, rect.y, frame_width, rect.height);

        float offset_x = frame_x + g_editor.atlas.padding - render_bounds.min.x * scale;
        float offset_y = rect.y + g_editor.atlas.padding - render_bounds.min.y * scale;

        int palette_index = g_editor.palette_map[mesh_impl->palette];

        for (int face_index = 0; face_index < frame->face_count; face_index++) {
            FaceData& face = frame->faces[face_index];
            if (face.vertex_count < 3) continue;

            SetColor(rasterizer,
                MultiplyAlpha(
                    g_editor.palettes[palette_index].colors[face.color],
                    face.opacity));
            BeginPath(rasterizer);
            AddFaceToPath(rasterizer, frame, face, offset_x, offset_y, scale);
            Fill(rasterizer);
        }
    }

    Free(rasterizer);
#endif
}

static void ScanPixelBounds(PixelData* pixels, int atlas_width, AtlasRect& rect) {
    int frame_width = rect.width / rect.frame_count;

    rect.pixel_min_x = rect.width;
    rect.pixel_min_y = rect.height;
    rect.pixel_max_x = 0;
    rect.pixel_max_y = 0;

    int frame_x = rect.x;
    for (int y = 0; y < rect.height; y++) {
        for (int x = 0; x < frame_width; x++) {
            int px = frame_x + x;
            int py = rect.y + y;
            int idx = (py * atlas_width + px);
            u8 alpha = pixels->rgba[idx].a;  // RGBA format, alpha is at offset 3

            if (alpha > 0) {
                if (x < rect.pixel_min_x) rect.pixel_min_x = x;
                if (y < rect.pixel_min_y) rect.pixel_min_y = y;
                if (x > rect.pixel_max_x) rect.pixel_max_x = x;
                if (y > rect.pixel_max_y) rect.pixel_max_y = y;
            }
        }
    }

    // If no pixels found, set to full rect
    if (rect.pixel_min_x > rect.pixel_max_x) {
        rect.pixel_min_x = 0;
        rect.pixel_min_y = 0;
        rect.pixel_max_x = frame_width - 1;
        rect.pixel_max_y = rect.height - 1;
    }
}

void SyncAtlasTexture(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels || !impl->dirty) return;

    if (!impl->texture) {
        impl->texture = CreateTexture(
            ALLOCATOR_DEFAULT,
            impl->pixels,
            impl->size.x,
            impl->size.y,
            TEXTURE_FORMAT_RGBA8,
            GetName("atlas"),
            TEXTURE_FILTER_NEAREST);
        impl->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
        SetTexture(impl->material, impl->texture, 0);
    } else {
        UpdateTexture(impl->texture, impl->pixels);
    }

    impl->dirty = false;
}

void RegenerateAtlas(AtlasData* atlas, PixelData* pixels) {
    AtlasDataImpl* impl = atlas->impl;

    // Use provided buffer or fall back to impl->pixels
    if (!pixels) {
        EnsurePixelBuffer(atlas);
        pixels = impl->pixels;
    }
    if (!pixels) return;

    // Clear pixel buffer
    memset(pixels, 0, impl->size.x * impl->size.y * 4);

    // Re-render all attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (mesh_asset) {
            MeshData* mesh = static_cast<MeshData*>(mesh_asset);
            RenderMeshToBuffer(atlas, mesh, impl->rects[i], pixels, true);
        }
    }

    if (!pixels) {
        impl->dirty = true;
    }
}

void RenderMeshToAtlas(AtlasData* atlas, MeshData* mesh, AtlasRect& rect, bool update_bounds) {
    EnsurePixelBuffer(atlas);
    RenderMeshToBuffer(atlas, mesh, rect, atlas->impl->pixels, update_bounds);
    ScanPixelBounds(atlas->impl->pixels, atlas->impl->size.x, rect);
    atlas->impl->dirty = true;
    atlas->impl->outline_dirty = true;
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

    ClearAllRects(atlas);

    Clear(impl->pixels);

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

Vec2 GetAtlasUV(AtlasData* atlas, const AtlasRect& rect, const Bounds2& mesh_bounds, const Vec2& position) {
    AtlasDataImpl* impl = atlas->impl;

    // Guard against zero-sized bounds (would cause division by zero)
    Vec2 size = GetSize(mesh_bounds);
    if (size.x < F32_EPSILON || size.y < F32_EPSILON) {
        LogError("GetAtlasUV: mesh_bounds has zero size! bounds=(%.3f,%.3f - %.3f,%.3f)",
            mesh_bounds.min.x, mesh_bounds.min.y, mesh_bounds.max.x, mesh_bounds.max.y);
        return {0.0f, 0.0f};
    }

    // Map position from mesh space to normalized [0,1] within mesh bounds
    float tx = (position.x - mesh_bounds.min.x) / size.x;
    float ty = (position.y - mesh_bounds.min.y) / size.y;

    // Map to actual pixel bounds (from pixel scan), sampling at texel centers
    // pixel_min/max are relative to frame origin, add rect.x/y for absolute position
    // Add 0.5 to sample at pixel centers, use (max - min) not (max - min + 1) to stay within bounds
    float pixel_x = rect.x + rect.pixel_min_x + 0.5f + tx * (rect.pixel_max_x - rect.pixel_min_x);
    float pixel_y = rect.y + rect.pixel_min_y + 0.5f + ty * (rect.pixel_max_y - rect.pixel_min_y);

    // Convert to UV
    float u = pixel_x / (float)impl->size.x;
    float v = pixel_y / (float)impl->size.y;

    return {u, v};
}

void GetExportQuadGeometry(AtlasData* atlas, const AtlasRect& rect,
    Vec2* out_min, Vec2* out_max,
    float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max) {

    float dpi = (float)atlas->impl->dpi;

    // Geometry at exact mesh_bounds so adjacent tiles meet perfectly
    *out_min = rect.mesh_bounds.min;
    *out_max = rect.mesh_bounds.max;

    // Calculate UV range that maps mesh_bounds to atlas pixels
    // UVs inset by 0.5 pixels to sample at texel centers (avoids edge sampling issues)
    Vec2 mesh_size = GetSize(rect.mesh_bounds);
    float expected_pixel_w = mesh_size.x * dpi;
    float expected_pixel_h = mesh_size.y * dpi;

    // Map mesh_bounds corners to atlas pixel coordinates, sampling at texel centers
    float pixel_at_min_x = rect.x + g_editor.atlas.padding + 0.5f;
    float pixel_at_min_y = rect.y + g_editor.atlas.padding + 0.5f;
    float pixel_at_max_x = rect.x + g_editor.atlas.padding + expected_pixel_w - 0.5f;
    float pixel_at_max_y = rect.y + g_editor.atlas.padding + expected_pixel_h - 0.5f;

    *out_u_min = pixel_at_min_x / (float)atlas->impl->size.x;
    *out_v_min = pixel_at_min_y / (float)atlas->impl->size.y;
    *out_u_max = pixel_at_max_x / (float)atlas->impl->size.x;
    *out_v_max = pixel_at_max_y / (float)atlas->impl->size.y;
}

void GetTightQuadGeometry(AtlasData* atlas, const AtlasRect& rect,
    Vec2* out_min, Vec2* out_max,
    float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max) {

    // For now, just use the full mesh_bounds like GetExportQuadGeometry
    // Tight bounds calculation is complex due to Y-flip between pixel and world space
    // and needs more careful implementation
    *out_min = rect.mesh_bounds.min;
    *out_max = rect.mesh_bounds.max;

    float dpi = (float)atlas->impl->dpi;
    Vec2 mesh_size = GetSize(rect.mesh_bounds);
    float expected_pixel_w = mesh_size.x * dpi;
    float expected_pixel_h = mesh_size.y * dpi;

    // Map mesh_bounds corners to atlas pixel coordinates, sampling at texel centers
    float pixel_at_min_x = rect.x + g_editor.atlas.padding + 0.5f;
    float pixel_at_min_y = rect.y + g_editor.atlas.padding + 0.5f;
    float pixel_at_max_x = rect.x + g_editor.atlas.padding + expected_pixel_w - 0.5f;
    float pixel_at_max_y = rect.y + g_editor.atlas.padding + expected_pixel_h - 0.5f;

    *out_u_min = pixel_at_min_x / (float)atlas->impl->size.x;
    *out_v_min = pixel_at_min_y / (float)atlas->impl->size.y;
    *out_u_max = pixel_at_max_x / (float)atlas->impl->size.x;
    *out_v_max = pixel_at_max_y / (float)atlas->impl->size.y;
}

// Helper to add a line segment as a quad to the mesh builder
static void AddLineQuad(MeshBuilder* builder, const Vec2& p0, const Vec2& p1, float thickness, const Vec4& color) {
    Vec2 dir = p1 - p0;
    float len = Length(dir);
    if (len < F32_EPSILON) return;
    dir = dir / len;
    Vec2 n = Perpendicular(dir);

    u16 base = GetVertexCount(builder);
    MeshVertex v = {};
    v.opacity = 1.0f;
    v.color = color;

    v.position = p0 - n * thickness; AddVertex(builder, v);
    v.position = p0 + n * thickness; AddVertex(builder, v);
    v.position = p1 + n * thickness; AddVertex(builder, v);
    v.position = p1 - n * thickness; AddVertex(builder, v);
    AddTriangle(builder, base + 0, base + 1, base + 3);
    AddTriangle(builder, base + 1, base + 2, base + 3);
}

Mesh* GetAtlasOutlineMesh(AtlasData* atlas) {
    if (!atlas || !atlas->impl) return nullptr;

    AtlasDataImpl* impl = atlas->impl;

    // Return cached mesh if not dirty and zoom hasn't changed
    if (impl->outline_mesh && !impl->outline_dirty && impl->outline_zoom_version == g_view.zoom_version) {
        return impl->outline_mesh;
    }

    // Free old mesh
    if (impl->outline_mesh) {
        Free(impl->outline_mesh);
        impl->outline_mesh = nullptr;
    }

    // Count valid rects and estimate edge count (pixel hull is typically small)
    int valid_count = 0;
    int total_edges = 0;
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;
        valid_count++;
        total_edges += 32;  // Conservative estimate for pixel hull edges
    }
    if (valid_count == 0) return nullptr;

    // Each edge is a quad (4 verts, 6 indices)
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, (u16)(total_edges * 4), (u16)(total_edges * 6));

    // Outline thickness - use zoom_ref_scale for consistent width at any zoom
    constexpr float LINE_WIDTH = 0.01f;
    float outline_size = LINE_WIDTH * g_view.zoom_ref_scale;

    constexpr float PIXELS_PER_UNIT = 51.2f;
    float pixel_to_world = 1.0f / PIXELS_PER_UNIT;
    Vec4 yellow = {1.0f, 1.0f, 0.0f, 1.0f};

    // Atlas display: centered at origin, Y flipped (atlas pixel Y=0 at display top)
    // display_x = (atlas_pixel_x - width/2) / PIXELS_PER_UNIT
    // display_y = (height/2 - atlas_pixel_y) / PIXELS_PER_UNIT
    float half_w = (float)impl->size.x * 0.5f;
    float half_h = (float)impl->size.y * 0.5f;

    for (int ri = 0; ri < impl->rect_count; ri++) {
        const AtlasRect& r = impl->rects[ri];
        if (!r.valid) continue;

        // Use tight rect around the atlas rect (with padding excluded)
        int frame_width = r.width / r.frame_count;
        float x0 = (float)(r.x + g_editor.atlas.padding);
        float y0 = (float)(r.y + g_editor.atlas.padding);
        float x1 = (float)(r.x + frame_width - g_editor.atlas.padding);
        float y1 = (float)(r.y + r.height - g_editor.atlas.padding);

        // Rect corners in atlas pixel space
        Vec2 corners[4] = {
            {x0, y0},  // top-left
            {x1, y0},  // top-right
            {x1, y1},  // bottom-right
            {x0, y1}   // bottom-left
        };

        // Draw rect edges - convert atlas pixel coords to display coords
        for (int i = 0; i < 4; i++) {
            Vec2 h0 = corners[i];
            Vec2 h1 = corners[(i + 1) % 4];

            // Atlas texture uses Scale(-Y), so display Y = half_h - atlas_y
            Vec2 d0 = { (h0.x - half_w) * pixel_to_world, (half_h - h0.y) * pixel_to_world };
            Vec2 d1 = { (h1.x - half_w) * pixel_to_world, (half_h - h1.y) * pixel_to_world };

            AddLineQuad(builder, d0, d1, outline_size, yellow);
        }
    }

    impl->outline_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, atlas->name, true);
    impl->outline_dirty = false;
    impl->outline_zoom_version = g_view.zoom_version;

    Free(builder);

    return impl->outline_mesh;
}

// Save/Load

static void SaveAtlasData(AssetData* a, const std::filesystem::path& path) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

    WriteCSTR(stream, "w %d\n", impl->size.x);
    WriteCSTR(stream, "h %d\n", impl->size.y);
    WriteCSTR(stream, "d %d\n", impl->dpi);
    WriteCSTR(stream, "\n");

    // Save rects: r "mesh_name" x y w h frame_count bounds_min_x bounds_min_y bounds_max_x bounds_max_y pixel_bounds
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            const AtlasRect& r = impl->rects[i];
            WriteCSTR(stream, "r \"%s\" %d %d %d %d %d %.6f %.6f %.6f %.6f %d %d %d %d\n",
                r.mesh_name->value,
                r.x, r.y, r.width, r.height, r.frame_count,
                r.mesh_bounds.min.x, r.mesh_bounds.min.y,
                r.mesh_bounds.max.x, r.mesh_bounds.max.y,
                r.pixel_min_x, r.pixel_min_y, r.pixel_max_x, r.pixel_max_y);
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

    // frame_count (required now, was optional before)
    int frame_count = 1;
    ExpectInt(tk, &frame_count);

    // mesh_bounds (optional for backwards compatibility)
    Bounds2 mesh_bounds = BOUNDS2_ZERO;
    float bmin_x = 0.0f;
    float bmin_y = 0.0f;
    float bmax_x = 0.0f;
    float bmax_y = 0.0f;
    bool has_bounds = ExpectFloat(tk, &bmin_x) && ExpectFloat(tk, &bmin_y) &&
                      ExpectFloat(tk, &bmax_x) && ExpectFloat(tk, &bmax_y);
    if (has_bounds) {
        mesh_bounds = {{bmin_x, bmin_y}, {bmax_x, bmax_y}};
    }

    // pixel_bounds (optional for backwards compatibility)
    int pixel_min_x = g_editor.atlas.padding;
    int pixel_min_y = g_editor.atlas.padding;
    int pixel_max_x = w / frame_count - g_editor.atlas.padding - 1;
    int pixel_max_y = h - g_editor.atlas.padding - 1;
    ExpectInt(tk, &pixel_min_x);
    ExpectInt(tk, &pixel_min_y);
    ExpectInt(tk, &pixel_max_x);
    ExpectInt(tk, &pixel_max_y);

    int slot = FindFreeRectSlot(impl);
    if (slot >= 0) {
        AtlasRect& rect = impl->rects[slot];
        rect.mesh_name = mesh_name;
        rect.x = x;
        rect.y = y;
        rect.width = w;
        rect.height = h;
        rect.frame_count = frame_count;
        rect.mesh_bounds = mesh_bounds;
        rect.pixel_min_x = pixel_min_x;
        rect.pixel_min_y = pixel_min_y;
        rect.pixel_max_x = pixel_max_x;
        rect.pixel_max_y = pixel_max_y;
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
            impl->size.x = ExpectInt(tk);
        } else if (ExpectIdentifier(tk, "h")) {
            impl->size.y = ExpectInt(tk);
        } else if (ExpectIdentifier(tk, "d")) {
            if (!ExpectInt(tk, &impl->dpi))
                ThrowError("missing atlas dpi");
        } else if (ExpectIdentifier(tk, "r")) {
            // Ensure packer is initialized with correct dimensions before parsing rects
            if (!impl->packer) {
                impl->packer = new RectPacker(impl->size.x, impl->size.y);
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
        impl->packer = new RectPacker(impl->size.x, impl->size.y);
    }

    // Use same scale as DrawAtlasData (512 pixels = 10 units for grid alignment)
    constexpr float PIXELS_PER_UNIT = 51.2f;
    Vec2 tsize = Vec2{static_cast<float>(impl->size.x), static_cast<float>(impl->size.y)} / PIXELS_PER_UNIT;
    a->bounds = Bounds2{-tsize.x*0.5f, -tsize.y*0.5f, tsize.x*0.5f, tsize.y*0.5f};
}

static void PostLoadAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Register as managed atlas if it has the auto-managed naming convention
    RegisterManagedAtlas(atlas);

    // Check for DPI mismatch - regenerate atlas if config DPI differs from saved DPI
    if (impl->dpi != g_editor.atlas.dpi) {
        LogInfo("Atlas '%s' DPI mismatch (file: %d, config: %d) - regenerating",
            a->name->value, impl->dpi, g_editor.atlas.dpi);
        impl->dpi = g_editor.atlas.dpi;
        RebuildAtlas(atlas);
        MarkModified(a);
        return;  // RebuildAtlas handles mesh binding and rendering
    }

    // Bind meshes to this atlas and render pixels
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                mesh->impl->atlas = atlas;

                // Check if bounds are valid (non-zero size)
                Vec2 size = GetSize(impl->rects[i].mesh_bounds);
                bool bounds_valid = size.x > F32_EPSILON && size.y > F32_EPSILON;

                // If bounds are invalid (old file format), update them from mesh
                // Otherwise preserve bounds loaded from atlas file to prevent race conditions
                RenderMeshToAtlas(atlas, mesh, impl->rects[i], !bounds_valid);
            }
        }
    }
}

static void LoadAtlasMetaData(AssetData* a, Props* meta) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    (void)meta;  // Width/height/dpi come from the .atlas file, not metadata

    InitAtlasEditor(atlas);
}

static void SaveAtlasMetaData(AssetData* a, Props* meta) {
    (void)a;
    (void)meta;
    // Width/height/dpi are saved in the .atlas file, not metadata
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
