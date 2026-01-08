//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "utils/rect_packer.h"
#include "utils/scanline_rasterizer.h"
#include "atlas_manager.h"

using namespace noz;

// Convert from PlutoVG's native-endian premultiplied ARGB to premultiplied RGBA
// (preserves premultiplication, unlike plutovg_convert_argb_to_rgba which un-premultiplies)
void ConvertARGBToRGBA(u8* dst, const u8* src, int width, int height) {
    const u32* src_pixels = (const u32*)src;
    for (int i = 0; i < width * height; i++) {
        u32 pixel = src_pixels[i];
        u8 a = (pixel >> 24) & 0xFF;
        u8 r = (pixel >> 16) & 0xFF;
        u8 g = (pixel >> 8) & 0xFF;
        u8 b = (pixel >> 0) & 0xFF;
        dst[i * 4 + 0] = r;
        dst[i * 4 + 1] = g;
        dst[i * 4 + 2] = b;
        dst[i * 4 + 3] = a;
    }
}

extern void InitAtlasEditor(AtlasData* atlas);

// Calculate bounds from vertices and curve control points in a frame
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

    // Include curve control points in bounds
    for (int i = 0; i < frame->edge_count; i++) {
        EdgeData& e = frame->edges[i];
        if (LengthSqr(e.curve_offset) > 0.0001f) {
            Vec2 p0 = frame->vertices[e.v0].position;
            Vec2 p1 = frame->vertices[e.v1].position;
            Vec2 control = (p0 + p1) * 0.5f + e.curve_offset;
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
    Vec2 size = Vec2{(float)impl->width, (float)impl->height} / PIXELS_PER_UNIT;

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
    atlas->impl->dpi = g_editor.atlas.dpi;
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
    int frame_width = (int)(frame_size.x * impl->dpi) + g_editor.atlas.padding * 2;
    int frame_height = (int)(frame_size.y * impl->dpi) + g_editor.atlas.padding * 2;

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
    int stride = impl->width * 4;
    for (int y = rect.y; y < rect.y + rect.height && y < impl->height; y++) {
        int offset = y * stride + rect.x * 4;
        int bytes = rect.width * 4;
        if (rect.x + rect.width > impl->width) {
            bytes = (impl->width - rect.x) * 4;
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

// Helper to find edge index between two vertices in a frame
static int GetFrameEdge(MeshFrameData* frame, int v0, int v1) {
    for (int i = 0; i < frame->edge_count; i++) {
        EdgeData& e = frame->edges[i];
        if ((e.v0 == v0 && e.v1 == v1) || (e.v0 == v1 && e.v1 == v0))
            return i;
    }
    return -1;
}

// Evaluate rational quadratic Bezier for atlas rendering
static Vec2 EvalRationalBezierAtlas(Vec2 p0, Vec2 control, Vec2 p1, float w, float t) {
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;
    float wt = 2.0f * u * t * w;
    float denom = u2 + wt + t2;
    return (p0 * u2 + control * wt + p1 * t2) / denom;
}

// Frame-based version for multi-frame rendering
static void AddFaceToPath(ScanlineRasterizer* rasterizer, MeshFrameData* frame, FaceData& face, float offset_x, float offset_y, float scale, float expand = 0.0f, bool snap_to_pixels = false) {
    constexpr int CURVE_SEGMENTS = 8;

    // Calculate face center for determining outward direction
    Vec2 center = {0, 0};
    for (int vi = 0; vi < face.vertex_count; vi++) {
        center = center + frame->vertices[face.vertices[vi]].position;
    }
    center = center / (float)face.vertex_count;

    for (int vi = 0; vi < face.vertex_count; vi++) {
        int v0_idx = face.vertices[vi];
        int v1_idx = face.vertices[(vi + 1) % face.vertex_count];
        Vec2 p0 = frame->vertices[v0_idx].position;
        Vec2 p1 = frame->vertices[v1_idx].position;

        // Check if this edge is shared (internal) - only expand shared edges to fill gaps
        int edge_idx = GetFrameEdge(frame, v0_idx, v1_idx);
        bool is_shared = edge_idx >= 0 && frame->edges[edge_idx].face_count > 1;

        // Calculate edge normal pointing outward from face center
        Vec2 edge_normal = {0, 0};
        if (expand > 0.0f && is_shared) {
            Vec2 edge_dir = Normalize(p1 - p0);
            edge_normal = {-edge_dir.y, edge_dir.x};
            Vec2 mid = (p0 + p1) * 0.5f;
            if (Dot(edge_normal, mid - center) < 0) edge_normal = -edge_normal;
            edge_normal = edge_normal * (expand / scale);
        }

        Vec2 pos0 = p0 + edge_normal;

        if (vi == 0) {
            float px = offset_x + pos0.x * scale;
            float py = offset_y + pos0.y * scale;
            // Snap to pixel corners for non-AA rendering to avoid sub-pixel artifacts
            if (snap_to_pixels) {
                px = roundf(px);
                py = roundf(py);
            }
            MoveTo(rasterizer, px, py);
        }

        // Check if edge has a curve
        if (edge_idx >= 0) {
            EdgeData& e = frame->edges[edge_idx];
            if (LengthSqr(e.curve_offset) > 0.0001f) {
                // Tessellate curved edge using rational bezier
                Vec2 control = (p0 + p1) * 0.5f + e.curve_offset;
                float w = e.curve_weight;

                // Flip curve direction if edge is reversed relative to face winding
                bool reversed = (e.v0 != v0_idx);

                for (int s = 1; s <= CURVE_SEGMENTS; s++) {
                    float t = (float)s / CURVE_SEGMENTS;
                    if (reversed) t = 1.0f - t;

                    Vec2 pt = EvalRationalBezierAtlas(p0, control, p1, w, reversed ? 1.0f - t : t);
                    pt = pt + edge_normal;  // Apply same expansion to curve points
                    float px = offset_x + pt.x * scale;
                    float py = offset_y + pt.y * scale;
                    if (snap_to_pixels) {
                        px = roundf(px);
                        py = roundf(py);
                    }
                    LineTo(rasterizer, px, py);
                }
                continue;
            }
        }

        // Straight edge
        Vec2 pos1 = p1 + edge_normal;
        float px = offset_x + pos1.x * scale;
        float py = offset_y + pos1.y * scale;
        if (snap_to_pixels) {
            px = roundf(px);
            py = roundf(py);
        }
        LineTo(rasterizer, px, py);
    }

    ClosePath(rasterizer);
}

void RenderMeshToBuffer(AtlasData* atlas, MeshData* mesh, AtlasRect& rect, u8* pixels, bool update_bounds) {
    AtlasDataImpl* impl = atlas->impl;
    MeshDataImpl* mesh_impl = mesh->impl;

    if (mesh_impl->frame_count == 0) return;

    // Calculate max bounds across all frames
    Bounds2 max_bounds = GetFrameBounds(&mesh_impl->frames[0]);
    for (int i = 1; i < mesh_impl->frame_count; i++) {
        max_bounds = Union(max_bounds, GetFrameBounds(&mesh_impl->frames[i]));
    }

    // Determine which bounds to use for rendering
    // When loading from file (update_bounds=false), use saved bounds for consistency
    // When updating (update_bounds=true), use fresh max_bounds
    Bounds2 render_bounds;
    if (update_bounds) {
        rect.mesh_bounds = max_bounds;
        render_bounds = max_bounds;
    } else {
        // Use saved bounds - ensures UV coords match what's rendered
        render_bounds = rect.mesh_bounds;
    }

    float scale = (float)impl->dpi;
    int frame_width = rect.width / rect.frame_count;

    // Set up rasterizer for the entire atlas
    ScanlineRasterizer* rasterizer = CreateScanlineRasterizer();
    SetTarget(rasterizer, pixels, impl->width, impl->height);
    SetAntialias(rasterizer, g_editor.atlas.antialias);

    // Render each frame at its position in the strip
    for (int frame_idx = 0; frame_idx < mesh_impl->frame_count; frame_idx++) {
        MeshFrameData* frame = &mesh_impl->frames[frame_idx];

        int frame_x = rect.x + frame_idx * frame_width;

        // Clip to this frame's region
        SetClipRect(rasterizer, frame_x, rect.y, frame_width, rect.height);

        // Use render_bounds for consistent positioning across all frames
        float offset_x = frame_x + g_editor.atlas.padding - render_bounds.min.x * scale;
        float offset_y = rect.y + g_editor.atlas.padding - render_bounds.min.y * scale;

        int palette_index = g_editor.palette_map[mesh_impl->palette];

        // Render each face individually (no batching to avoid even-odd fill issues with shared edges)
        bool snap_to_pixels = !g_editor.atlas.antialias;
        for (int fi = 0; fi < frame->face_count; fi++) {
            FaceData& face = frame->faces[fi];
            if (face.vertex_count < 3) continue;

            int color_index = face.color;
            float face_opacity = face.opacity;
            Color color = g_editor.palettes[palette_index].colors[color_index];
            float final_alpha = color.a * face_opacity;

            SetColor(rasterizer, color.r, color.g, color.b, final_alpha);
            BeginPath(rasterizer);
            // Expansion fills gaps at internal coplanar edges (only applied to shared edges)
            AddFaceToPath(rasterizer, frame, face, offset_x, offset_y, scale, 0.75f, snap_to_pixels);
            Fill(rasterizer);
        }
    }

    DestroyScanlineRasterizer(rasterizer);

    // Dilate pixels to prevent bilinear filtering from sampling empty (black) pixels
    // Skip dilation when using AA/premultiplied - premultiplied blending handles edges correctly
    // and dilation would cause bright halos instead
    if (!g_editor.atlas.antialias) {
        for (int frame_idx = 0; frame_idx < mesh_impl->frame_count; frame_idx++) {
            int frame_x = rect.x + frame_idx * frame_width;

            for (int y = rect.y; y < rect.y + rect.height; y++) {
                for (int x = frame_x; x < frame_x + frame_width; x++) {
                    u32* pixel = (u32*)(pixels + (y * impl->width + x) * 4);
                    if ((*pixel >> 24) != 0) continue; // Already filled

                    // Check 4 neighbors for a filled pixel to copy from
                    static const int dx[] = {-1, 1, 0, 0};
                    static const int dy[] = {0, 0, -1, 1};
                    for (int d = 0; d < 4; d++) {
                        int nx = x + dx[d];
                        int ny = y + dy[d];
                        if (nx < frame_x || nx >= frame_x + frame_width) continue;
                        if (ny < rect.y || ny >= rect.y + rect.height) continue;

                        u32* neighbor = (u32*)(pixels + (ny * impl->width + nx) * 4);
                        if ((*neighbor >> 24) != 0) {
                            // Copy color but set alpha to 0 (for proper blending at edges)
                            *pixel = *neighbor & 0x00FFFFFF;
                            break;
                        }
                    }
                }
            }
        }
    }
}

// Scan rendered pixels to find actual content bounds (non-zero alpha)
static void ScanPixelBounds(u8* pixels, int atlas_width, AtlasRect& rect) {
    int frame_width = rect.width / rect.frame_count;

    // Initialize to invalid bounds (will be set on first pixel found)
    rect.pixel_min_x = rect.width;
    rect.pixel_min_y = rect.height;
    rect.pixel_max_x = 0;
    rect.pixel_max_y = 0;

    // Only scan first frame for bounds (all frames should have similar bounds)
    int frame_x = rect.x;

    for (int y = 0; y < rect.height; y++) {
        for (int x = 0; x < frame_width; x++) {
            int px = frame_x + x;
            int py = rect.y + y;
            int idx = (py * atlas_width + px) * 4;
            u8 alpha = pixels[idx + 3];  // ARGB format, alpha is at offset 3

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

void RenderMeshToAtlas(AtlasData* atlas, MeshData* mesh, AtlasRect& rect, bool update_bounds) {
    EnsurePixelBuffer(atlas);
    RenderMeshToBuffer(atlas, mesh, rect, atlas->impl->pixels, update_bounds);
    ScanPixelBounds(atlas->impl->pixels, atlas->impl->width, rect);
    atlas->impl->dirty = true;
    atlas->impl->outline_dirty = true;
}

void SyncAtlasTexture(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels || !impl->dirty) return;

    // Convert from premultiplied ARGB to premultiplied RGBA for GPU upload
    u32 buffer_size = impl->width * impl->height * 4;
    u8* rgba_pixels = static_cast<u8 *>(Alloc(ALLOCATOR_SCRATCH, buffer_size));
    ConvertARGBToRGBA(rgba_pixels, impl->pixels, impl->width, impl->height);

    if (!impl->texture) {
        impl->texture = CreateTexture(
            ALLOCATOR_DEFAULT,
            rgba_pixels,
            impl->width,
            impl->height,
            TEXTURE_FORMAT_RGBA8,
            GetName("atlas"),
            TEXTURE_FILTER_NEAREST);
        impl->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
        SetTexture(impl->material, impl->texture, 0);
    } else {
        UpdateTexture(impl->texture, rgba_pixels);
    }

    impl->dirty = false;
}

void RegenerateAtlas(AtlasData* atlas, u8* buffer) {
    AtlasDataImpl* impl = atlas->impl;

    // Use provided buffer or fall back to impl->pixels
    u8* pixels = buffer;
    if (!pixels) {
        EnsurePixelBuffer(atlas);
        pixels = impl->pixels;
    }
    if (!pixels) return;

    // Clear pixel buffer
    memset(pixels, 0, impl->width * impl->height * 4);

    // Re-render all attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (mesh_asset) {
            MeshData* mesh = static_cast<MeshData*>(mesh_asset);
            RenderMeshToBuffer(atlas, mesh, impl->rects[i], pixels, true);
        }
    }

    if (!buffer) {
        impl->dirty = true;
    }
}

void DilateAtlasRect(u8* pixels, int atlas_width, int atlas_height, const AtlasRect& rect) {
    if (rect.frame_count <= 0 || rect.width <= 0 || rect.height <= 0) return;
    if (rect.height <= g_editor.atlas.padding * 2) return;  // Too small to have content

    int frame_width = rect.width / rect.frame_count;
    if (frame_width <= g_editor.atlas.padding * 2) return;  // Too small to have content

    for (int frame_idx = 0; frame_idx < rect.frame_count; frame_idx++) {
        int frame_x = rect.x + frame_idx * frame_width;

        // Content area within the rect (after padding)
        int content_x = frame_x + g_editor.atlas.padding;
        int content_y = rect.y + g_editor.atlas.padding;
        int content_w = frame_width - g_editor.atlas.padding * 2;
        int content_h = rect.height - g_editor.atlas.padding * 2;
        int stride = atlas_width * 4;

        // Bounds check - ensure we don't write outside atlas
        if (content_y - 1 < 0 || content_y + content_h >= atlas_height) continue;
        if (content_x - 1 < 0 || content_x + content_w >= atlas_width) continue;

        // Sample from 1 pixel inside to avoid AA edge pixels (EXPAND pushes content 0.5px outward)
        int src_x = content_x + 1;
        int src_y = content_y + 1;
        int src_w = content_w - 2;
        int src_h = content_h - 2;
        if (src_w <= 0 || src_h <= 0) continue;

        // Top edge: copy row src_y to row content_y-1
        memcpy(pixels + (content_y - 1) * stride + content_x * 4,
               pixels + src_y * stride + content_x * 4,
               content_w * 4);

        // Bottom edge: copy row src_y+src_h-1 to row content_y+content_h
        memcpy(pixels + (content_y + content_h) * stride + content_x * 4,
               pixels + (src_y + src_h - 1) * stride + content_x * 4,
               content_w * 4);

        // Left edge: copy column src_x to column content_x-1
        for (int y = content_y; y < content_y + content_h; y++) {
            memcpy(pixels + y * stride + (content_x - 1) * 4,
                   pixels + y * stride + src_x * 4, 4);
        }

        // Right edge: copy column src_x+src_w-1 to column content_x+content_w
        for (int y = content_y; y < content_y + content_h; y++) {
            memcpy(pixels + y * stride + (content_x + content_w) * 4,
                   pixels + y * stride + (src_x + src_w - 1) * 4, 4);
        }

        // Corners - sample from inside corners
        memcpy(pixels + (content_y - 1) * stride + (content_x - 1) * 4,
               pixels + src_y * stride + src_x * 4, 4);  // Top-left
        memcpy(pixels + (content_y - 1) * stride + (content_x + content_w) * 4,
               pixels + src_y * stride + (src_x + src_w - 1) * 4, 4);  // Top-right
        memcpy(pixels + (content_y + content_h) * stride + (content_x - 1) * 4,
               pixels + (src_y + src_h - 1) * stride + src_x * 4, 4);  // Bottom-left
        memcpy(pixels + (content_y + content_h) * stride + (content_x + content_w) * 4,
               pixels + (src_y + src_h - 1) * stride + (src_x + src_w - 1) * 4, 4);  // Bottom-right
    }
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

// UV computation and export mesh visualization

// Check if mesh is skinned (has a skeleton or any bone weights) AND has valid geometry
bool IsSkinnedMesh(MeshData* mesh) {
    if (!mesh || !mesh->impl) return false;
    if (mesh->impl->frame_count == 0) return false;

    // Must have face data to be triangulated
    MeshFrameData* frame = &mesh->impl->frames[0];
    if (frame->face_count == 0 || frame->vertex_count == 0) return false;

    // Check skeleton reference or cached bone count (updated in UpdateEdges)
    return mesh->impl->skeleton_name != nullptr || mesh->impl->bone_count > 0;
}

// Check if all vertices are weighted to exactly one bone, return that bone index (-1 if not single-bone)
int GetSingleBoneIndex(MeshData* mesh) {
    if (!mesh || !mesh->impl || mesh->impl->bone_count != 1) return -1;

    MeshFrameData* frame = &mesh->impl->frames[0];
    int single_bone = -1;

    for (int vi = 0; vi < frame->vertex_count; vi++) {
        const VertexData& v = frame->vertices[vi];
        int bone_idx = -1;
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            if (v.weights[w].weight > F32_EPSILON) {
                if (bone_idx >= 0 && bone_idx != v.weights[w].bone_index) {
                    return -1;  // Multiple bones on this vertex
                }
                bone_idx = v.weights[w].bone_index;
            }
        }
        if (bone_idx >= 0) {
            if (single_bone < 0) {
                single_bone = bone_idx;
            } else if (single_bone != bone_idx) {
                return -1;  // Different vertices weighted to different bones
            }
        }
    }
    return single_bone;
}

// Expand hull vertices outward along edge normals (proper polygon inflation)
// Returns expanded positions in out_positions array
void ExpandHullByEdgeNormals(MeshData* mesh, const int* hull_indices, int hull_count, float expand, Vec2* out_positions) {
    MeshFrameData* frame = &mesh->impl->frames[0];

    for (int i = 0; i < hull_count; i++) {
        // Get current vertex and adjacent edges
        Vec2 curr = frame->vertices[hull_indices[i]].position;
        Vec2 prev = frame->vertices[hull_indices[(i + hull_count - 1) % hull_count]].position;
        Vec2 next = frame->vertices[hull_indices[(i + 1) % hull_count]].position;

        // Compute outward normals for both adjacent edges
        Vec2 edge_prev = curr - prev;
        Vec2 edge_next = next - curr;

        // Perpendicular (rotate 90 degrees CW for outward normal on CW hull from gift wrap)
        Vec2 n_prev = Normalize(Vec2{edge_prev.y, -edge_prev.x});
        Vec2 n_next = Normalize(Vec2{edge_next.y, -edge_next.x});

        // Average the normals for the vertex offset direction
        Vec2 n_avg = Normalize(n_prev + n_next);

        // Compute how much to scale the offset to maintain edge distance
        // (miter calculation - accounts for angle between edges)
        float dot = Dot(n_avg, n_prev);
        float scale = (dot > 0.1f) ? (1.0f / dot) : 1.0f;  // Clamp to avoid extreme miter
        scale = Min(scale, 3.0f);  // Limit max miter

        out_positions[i] = curr + n_avg * expand * scale;
    }
}

// Compute convex hull of mesh vertices using gift wrapping algorithm
// Returns hull vertex indices in CCW order
int ComputeConvexHull(MeshData* mesh, int* hull_indices, int max_hull) {
    MeshFrameData* frame = &mesh->impl->frames[0];
    if (frame->vertex_count < 3) return 0;

    // Find leftmost point
    int start = 0;
    for (int i = 1; i < frame->vertex_count; i++) {
        if (frame->vertices[i].position.x < frame->vertices[start].position.x) {
            start = i;
        }
    }

    int hull_count = 0;
    int current = start;

    do {
        if (hull_count >= max_hull) break;
        hull_indices[hull_count++] = current;

        int next = 0;
        for (int i = 1; i < frame->vertex_count; i++) {
            if (i == current) continue;
            if (next == current) {
                next = i;
                continue;
            }

            Vec2 a = frame->vertices[current].position;
            Vec2 b = frame->vertices[next].position;
            Vec2 c = frame->vertices[i].position;

            // Cross product to determine turn direction
            float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (cross < 0 || (cross == 0 && LengthSqr(c - a) > LengthSqr(b - a))) {
                next = i;
            }
        }
        current = next;
    } while (current != start && hull_count < max_hull);

    return hull_count;
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
    float u = pixel_x / (float)impl->width;
    float v = pixel_y / (float)impl->height;

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

    *out_u_min = pixel_at_min_x / (float)atlas->impl->width;
    *out_v_min = pixel_at_min_y / (float)atlas->impl->height;
    *out_u_max = pixel_at_max_x / (float)atlas->impl->width;
    *out_v_max = pixel_at_max_y / (float)atlas->impl->height;
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

    // Count valid rects and estimate edge count
    int valid_count = 0;
    int total_edges = 0;
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;
        valid_count++;

        // Get mesh to check if skinned
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (mesh_asset) {
            MeshData* mesh = static_cast<MeshData*>(mesh_asset);
            if (IsSkinnedMesh(mesh)) {
                int single_bone = GetSingleBoneIndex(mesh);
                if (single_bone >= 0) {
                    // Single-bone: hull edges (estimate vertex count as upper bound)
                    total_edges += mesh->impl->frames[0].vertex_count;
                } else {
                    // Multi-bone: triangulated edges
                    Mesh* tri_mesh = ToMesh(mesh, false, true);
                    if (tri_mesh) {
                        total_edges += GetIndexCount(tri_mesh);
                    }
                }
            } else {
                total_edges += 4;  // Quad: 4 edges
            }
        } else {
            total_edges += 4;  // Default quad
        }
    }
    if (valid_count == 0) return nullptr;

    // Each edge is a quad (4 verts, 6 indices)
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, (u16)(total_edges * 4), (u16)(total_edges * 6));

    float dpi = (float)impl->dpi;

    // Outline thickness - use zoom_ref_scale for consistent width at any zoom
    constexpr float LINE_WIDTH = 0.01f;
    float outline_size = LINE_WIDTH * g_view.zoom_ref_scale;

    constexpr float PIXELS_PER_UNIT = 51.2f;
    float pixel_to_world = 1.0f / PIXELS_PER_UNIT;
    Vec4 yellow = {1.0f, 1.0f, 0.0f, 1.0f};

    for (int ri = 0; ri < impl->rect_count; ri++) {
        const AtlasRect& r = impl->rects[ri];
        if (!r.valid) continue;

        // Transform from mesh space to atlas display space
        float mesh_to_world = dpi * pixel_to_world;
        Vec2 atlas_size = Vec2{(float)impl->width, (float)impl->height} * pixel_to_world;
        Vec2 atlas_bottom_left = -atlas_size * 0.5f;
        Vec2 mesh_offset = atlas_bottom_left +
            Vec2{(float)(r.x + g_editor.atlas.padding), (float)(r.y + g_editor.atlas.padding)} * pixel_to_world -
            r.mesh_bounds.min * mesh_to_world;

        // Get mesh to check if skinned
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, r.mesh_name);
        MeshData* mesh_data = mesh_asset ? static_cast<MeshData*>(mesh_asset) : nullptr;

        if (mesh_data && IsSkinnedMesh(mesh_data)) {
            int single_bone = GetSingleBoneIndex(mesh_data);
            if (single_bone >= 0) {
                // Single-bone mesh: draw convex hull outline (with edge-normal expansion)
                int hull_indices[MESH_MAX_VERTICES];
                int hull_count = ComputeConvexHull(mesh_data, hull_indices, MESH_MAX_VERTICES);

                // Expand hull using edge normals (1.5 pixels to cover edges and corners)
                float expand = ATLAS_HULL_EXPAND / dpi;
                Vec2 expanded[MESH_MAX_VERTICES];
                ExpandHullByEdgeNormals(mesh_data, hull_indices, hull_count, expand, expanded);

                for (int i = 0; i < hull_count; i++) {
                    Vec2 p0 = expanded[i];
                    Vec2 p1 = expanded[(i + 1) % hull_count];

                    p0 = p0 * mesh_to_world + mesh_offset;
                    p1 = p1 * mesh_to_world + mesh_offset;

                    AddLineQuad(builder, p0, p1, outline_size, yellow);
                }
            } else {
                // Multi-bone skinned mesh: draw actual triangulated mesh edges
                Mesh* tri_mesh = ToMesh(mesh_data, false, true);
                if (tri_mesh) {
                    const MeshVertex* verts = GetVertices(tri_mesh);
                    const u16* indices = GetIndices(tri_mesh);
                    u16 index_count = GetIndexCount(tri_mesh);

                    // Draw each triangle edge
                    for (int i = 0; i < index_count; i += 3) {
                        for (int e = 0; e < 3; e++) {
                            Vec2 p0 = verts[indices[i + e]].position;
                            Vec2 p1 = verts[indices[i + (e + 1) % 3]].position;

                            p0 = p0 * mesh_to_world + mesh_offset;
                            p1 = p1 * mesh_to_world + mesh_offset;

                            AddLineQuad(builder, p0, p1, outline_size, yellow);
                        }
                    }
                }
            }
        } else {
            // Non-skinned: draw quad outline using shared export geometry
            Vec2 mesh_min, mesh_max;
            float u_min, v_min, u_max, v_max;
            GetExportQuadGeometry(atlas, r, &mesh_min, &mesh_max, &u_min, &v_min, &u_max, &v_max);

            Vec2 corners[4] = {
                Vec2{mesh_min.x, mesh_min.y} * mesh_to_world + mesh_offset,
                Vec2{mesh_max.x, mesh_min.y} * mesh_to_world + mesh_offset,
                Vec2{mesh_max.x, mesh_max.y} * mesh_to_world + mesh_offset,
                Vec2{mesh_min.x, mesh_max.y} * mesh_to_world + mesh_offset
            };

            for (int i = 0; i < 4; i++) {
                AddLineQuad(builder, corners[i], corners[(i + 1) % 4], outline_size, yellow);
            }
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

    WriteCSTR(stream, "w %d\n", impl->width);
    WriteCSTR(stream, "h %d\n", impl->height);
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

    // Use same scale as DrawAtlasData (512 pixels = 10 units for grid alignment)
    constexpr float PIXELS_PER_UNIT = 51.2f;
    Vec2 tsize = Vec2{static_cast<float>(impl->width), static_cast<float>(impl->height)} / PIXELS_PER_UNIT;
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
