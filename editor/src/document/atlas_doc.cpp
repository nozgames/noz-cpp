//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "utils/pixel_data.h"
#include "utils/rect_packer.h"
#include "atlas_manager.h"
#include "sprite_doc.h"

namespace noz::editor {

    extern void InitAtlasEditor(AtlasDocument* adoc);

    static void DrawAtlasData(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ATLAS);

        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        DrawBounds(doc, 0.05f);

        // Sync pixels to GPU if dirty
        SyncAtlasTexture(adoc);

        // Scale factor to display atlas in world units (512 pixels = 10 units for grid alignment)
        constexpr float PIXELS_PER_UNIT = 51.2f;
        Vec2 size = ToVec2(adoc->size) / PIXELS_PER_UNIT;

        // Draw the atlas texture if available
        if (adoc->material) {
            BindDepth(-0.1f);
            BindColor(COLOR_WHITE);
            BindMaterial(adoc->material);
            // Flip Y because texture coords are top-down
            DrawMesh(g_workspace.quad_mesh, Translate(doc->position) * Scale(Vec2{size.x, -size.y}));
        } else {
            // Draw a gray placeholder
            BindDepth(0.0f);
            BindMaterial(g_workspace.editor_material);
            BindColor(Color{0.2f, 0.2f, 0.2f, 1.0f});
            DrawMesh(g_workspace.quad_mesh, Translate(doc->position) * Scale(size));
        }
    }

    static void DestroyAtlasDocument(Document* doc) {
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        UnregisterManagedAtlas(adoc);

        if (!adoc) return;

        Free(adoc->pixels);
        Free(adoc->outline_mesh);
        delete adoc->packer;

        adoc->pixels = nullptr;
        adoc->packer = nullptr;
        adoc->outline_mesh = nullptr;
    }

    static void CLoneAtlasDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_ATLAS);
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        int old_width = adoc->size.x;
        int old_height = adoc->size.y;
        adoc->size.x = ATLAS_DEFAULT_SIZE;
        adoc->size.y = ATLAS_DEFAULT_SIZE;
        adoc->dpi = g_editor.atlas.dpi;
        adoc->dirty = true;
        adoc->pixels = nullptr;
        adoc->texture = nullptr;
        adoc->material = nullptr;
        adoc->packer = new RectPacker(old_width, old_height);
        adoc->dirty = true;

        // Mark existing rects as used in new packer
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid) {
                const AtlasRect& r = adoc->rects[i];
                RectPacker::Rect bin_rect(r.x, r.y, r.width, r.height);
                adoc->packer->MarkUsed(bin_rect);
            }
        }
    }

    AtlasRect* FindRectForSprite(AtlasDocument* adoc, const Name* sprite_name) {
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid && adoc->rects[i].asset_name == sprite_name) {
                return &adoc->rects[i];
            }
        }
        return nullptr;
    }

    AtlasDocument* FindAtlasForSprite(const Name* sprite_name, AtlasRect** out_rect) {
        // Search all atlas assets for one containing this sprite
        for (int i = 0, count = GetDocumentCount(); i < count; i++) {
            Document* asset = GetDocument(i);
            if (asset->def->type != ASSET_TYPE_ATLAS) continue;

            AtlasDocument* atlas = static_cast<AtlasDocument*>(asset);
            AtlasRect* rect = FindRectForSprite(atlas, sprite_name);
            if (rect) {
                if (out_rect) *out_rect = rect;
                return atlas;
            }
        }
        return nullptr;
    }

    static int FindFreeRectSlot(AtlasDocument* adoc) {
        for (int i = 0; i < adoc->rect_count; i++) {
            if (!adoc->rects[i].valid) return i;
        }
        if (adoc->rect_count < ATLAS_MAX_RECTS) {
            return adoc->rect_count++;
        }
        return -1;
    }

    void FreeRect(AtlasDocument* adoc, AtlasRect* rect) {
        if (!rect) return;

        if (rect->asset_name) {
            SpriteDocument* sdoc = static_cast<SpriteDocument*>(FindDocument(ASSET_TYPE_SPRITE, rect->asset_name));
            if (sdoc)
                sdoc->atlas = nullptr;
        }

        rect->valid = false;
        rect->asset_name = nullptr;
        rect->frame_count = 1;
        adoc->dirty = true;
        adoc->outline_dirty = true;
    }

    void ClearRectPixels(AtlasDocument* adoc, const AtlasRect& rect) {
        if (!adoc->pixels) return;

        // Clear each row of the rect to transparent black
        int stride = adoc->size.x * 4;
        for (int y = rect.y; y < rect.y + rect.height && y < adoc->size.y; y++) {
            int offset = y * stride + rect.x * 4;
            int bytes = rect.width * 4;
            if (rect.x + rect.width > adoc->size.x) {
                bytes = (adoc->size.x - rect.x) * 4;
            }
            memset(adoc->pixels + offset, 0, bytes);
        }
        adoc->dirty = true;
    }

    void ClearAllRects(AtlasDocument* adoc) {
        for (int i = 0; i < adoc->rect_count; i++) {
            // Clear the asset's atlas reference
            if (adoc->rects[i].valid && adoc->rects[i].asset_name) {
                SpriteDocument* sdoc = static_cast<SpriteDocument*>(FindDocument(ASSET_TYPE_SPRITE, adoc->rects[i].asset_name));
                if (sdoc)
                    sdoc->atlas = nullptr;
            }
            adoc->rects[i].valid = false;
            adoc->rects[i].asset_name = nullptr;
            adoc->rects[i].frame_count = 1;
        }
        adoc->rect_count = 0;
        adoc->dirty = true;
        adoc->outline_dirty = true;

        // Reset packer
        if (adoc->packer) {
            delete adoc->packer;
        }
        adoc->packer = new RectPacker(adoc->size.x, adoc->size.y);
    }

    static void EnsurePixelBuffer(AtlasDocument* adoc) {
        if (!adoc->pixels)
            adoc->pixels = CreatePixelData(ALLOCATOR_DEFAULT, adoc->size);
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
                u8 alpha = pixels->rgba[idx].a;

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

    void SyncAtlasTexture(AtlasDocument* adoc) {
        if (!adoc->pixels || !adoc->dirty) return;

        if (!adoc->texture) {
            adoc->texture = CreateTexture(
                ALLOCATOR_DEFAULT,
                adoc->pixels->rgba,
                adoc->size.x,
                adoc->size.y,
                TEXTURE_FORMAT_RGBA8,
                GetName("atlas"),
                TEXTURE_FILTER_NEAREST);
            adoc->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
            SetTexture(adoc->material, adoc->texture, 0);
        } else {
            UpdateTexture(adoc->texture, adoc->pixels->rgba);
        }

        adoc->dirty = false;
    }

    void RegenerateAtlas(AtlasDocument* adoc, PixelData* pixels) {
        // Use provided buffer or fall back to adoc->pixels
        if (!pixels) {
            EnsurePixelBuffer(adoc);
            pixels = adoc->pixels;
        }
        if (!pixels) return;

        // Clear pixel buffer
        Clear(pixels);

        // Re-render all attached sprites
        for (int i = 0; i < adoc->rect_count; i++) {
            if (!adoc->rects[i].valid) continue;

            SpriteDocument* sdoc = static_cast<SpriteDocument*>(FindDocument(ASSET_TYPE_SPRITE, adoc->rects[i].asset_name));
            if (sdoc)
                RenderSpriteToAtlas(adoc, sdoc, adoc->rects[i], true);
        }

        adoc->dirty = true;
    }

    // Calculate max raster bounds across all sprite frames
    static RectInt GetMaxSpriteRasterBounds(SpriteDocument* sdoc) {
        if (sdoc->frame_count == 0) {
            return {0, 0, 0, 0};
        }

        // Update samples and bounds for all frames
        for (u16 fi = 0; fi < sdoc->frame_count; ++fi) {
            shape::UpdateSamples(&sdoc->frames[fi].shape);
            shape::UpdateBounds(&sdoc->frames[fi].shape);
        }

        RectInt max_bounds = sdoc->frames[0].shape.raster_bounds;
        for (u16 fi = 1; fi < sdoc->frame_count; ++fi) {
            RectInt& rb = sdoc->frames[fi].shape.raster_bounds;
            int min_x = Min(max_bounds.x, rb.x);
            int min_y = Min(max_bounds.y, rb.y);
            int max_x = Max(max_bounds.x + max_bounds.w, rb.x + rb.w);
            int max_y = Max(max_bounds.y + max_bounds.h, rb.y + rb.h);
            max_bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
        }

        return max_bounds;
    }

    AtlasRect* AllocateRect(AtlasDocument* adoc, SpriteDocument* sdoc) {
        if (sdoc->frame_count == 0) return nullptr;

        // Calculate max raster bounds across all frames
        RectInt max_bounds = GetMaxSpriteRasterBounds(sdoc);
        if (max_bounds.w <= 0 || max_bounds.h <= 0) return nullptr;

        int frame_width = max_bounds.w + g_editor.atlas.padding * 2;
        int frame_height = max_bounds.h + g_editor.atlas.padding * 2;

        // Total strip size: frames laid out horizontally
        int total_width = frame_width * sdoc->frame_count;
        int total_height = frame_height;

        if (total_width > adoc->size.x || total_height > adoc->size.y) {
            return nullptr;  // Too large for atlas
        }

        int slot = FindFreeRectSlot(adoc);
        if (slot < 0) return nullptr;

        RectPacker::Rect bin_rect;
        int result = adoc->packer->Insert(total_width, total_height, RectPacker::method::BottomLeftRule, bin_rect);
        if (result < 0) {
            return nullptr;
        }

        // Use precise shape bounds (not raster_bounds which are floored/ceiled)
        Bounds2 shape_bounds = sdoc->frames[0].shape.bounds;
        for (u16 fi = 1; fi < sdoc->frame_count; ++fi) {
            shape_bounds = Union(shape_bounds, sdoc->frames[fi].shape.bounds);
        }

        AtlasRect& rect = adoc->rects[slot];
        rect.x = bin_rect.x;
        rect.y = bin_rect.y;
        rect.width = bin_rect.w;
        rect.height = bin_rect.h;
        rect.asset_name = sdoc->name;
        rect.valid = true;
        rect.frame_count = sdoc->frame_count;
        rect.bounds = shape_bounds;

        adoc->dirty = true;
        adoc->outline_dirty = true;
        return &rect;
    }

    void RenderSpriteToAtlas(AtlasDocument* adoc, SpriteDocument* sdoc, AtlasRect& rect, bool update_bounds) {
        EnsurePixelBuffer(adoc);

        if (sdoc->frame_count == 0) return;

        // Calculate max bounds across all frames for consistent positioning
        RectInt max_bounds = GetMaxSpriteRasterBounds(sdoc);
        if (max_bounds.w <= 0 || max_bounds.h <= 0) return;

        if (update_bounds) {
            // Use the precise shape bounds (not raster_bounds which are floored/ceiled)
            // This ensures geometry matches the original sprite exactly
            Bounds2 shape_bounds = sdoc->frames[0].shape.bounds;
            for (u16 fi = 1; fi < sdoc->frame_count; ++fi) {
                shape_bounds = Union(shape_bounds, sdoc->frames[fi].shape.bounds);
            }
            rect.bounds = shape_bounds;
        }

        int frame_width = rect.width / rect.frame_count;
        int palette_index = g_editor.palette_map[sdoc->palette];
        const Color* palette = g_editor.palettes[palette_index].colors;

        for (int frame_idx = 0; frame_idx < sdoc->frame_count; frame_idx++) {
            SpriteFrame* f = &sdoc->frames[frame_idx];
            shape::Shape* shape = &f->shape;

            int frame_x = rect.x + frame_idx * frame_width;

            // Calculate offset to map shape's raster_bounds to atlas position
            // Rasterize adds rb.x/y internally, so we subtract max_bounds to align
            // all frames relative to the union bounds, then add atlas position + padding
            Vec2Int offset = {
                frame_x + g_editor.atlas.padding - max_bounds.x,
                rect.y + g_editor.atlas.padding - max_bounds.y
            };

            shape::Rasterize(shape, adoc->pixels, palette, offset);
        }

        ScanPixelBounds(adoc->pixels, adoc->size.x, rect);
        adoc->dirty = true;
        adoc->outline_dirty = true;
    }

    void RebuildAtlas(AtlasDocument* adoc) {
        struct RectInfo {
            const Name* name;
            int frame_count;
        };
        RectInfo rect_infos[ATLAS_MAX_RECTS];
        int rect_info_count = 0;
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid && adoc->rects[i].asset_name) {
                rect_infos[rect_info_count].name = adoc->rects[i].asset_name;
                rect_infos[rect_info_count].frame_count = adoc->rects[i].frame_count;
                rect_info_count++;
            }
        }

        int original_count = rect_info_count;

        ClearAllRects(adoc);

        EnsurePixelBuffer(adoc);
        Clear(adoc->pixels);

        // Re-allocate and render each sprite
        int success_count = 0;
        for (int i = 0; i < rect_info_count; i++) {
            SpriteDocument* sdoc = static_cast<SpriteDocument*>(FindDocument(ASSET_TYPE_SPRITE, rect_infos[i].name));
            if (!sdoc) {
                LogError("RebuildAtlas: sprite '%s' not found", rect_infos[i].name->value);
                continue;
            }

            AtlasRect* rect = AllocateRect(adoc, sdoc);
            if (rect) {
                RenderSpriteToAtlas(adoc, sdoc, *rect);
                success_count++;
            } else {
                LogError("RebuildAtlas: failed to allocate rect for '%s'", rect_infos[i].name->value);
            }
            MarkModified(sdoc);
        }

        if (success_count < original_count) {
            LogWarning("RebuildAtlas: only %d of %d sprites were successfully added", success_count, original_count);
        }

        adoc->dirty = true;
    }

    Vec2 GetAtlasUV(AtlasDocument* adoc, const AtlasRect& rect, const Bounds2& bounds, const Vec2& position) {
        // Guard against zero-sized bounds (would cause division by zero)
        Vec2 size = GetSize(bounds);
        if (size.x < F32_EPSILON || size.y < F32_EPSILON) {
            LogError("GetAtlasUV: bounds has zero size! bounds=(%.3f,%.3f - %.3f,%.3f)",
                bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
            return {0.0f, 0.0f};
        }

        // Map position from sprite space to normalized [0,1] within bounds
        float tx = (position.x - bounds.min.x) / size.x;
        float ty = (position.y - bounds.min.y) / size.y;

        // Map to actual pixel bounds (from pixel scan), sampling at texel centers
        // pixel_min/max are relative to frame origin, add rect.x/y for absolute position
        // Add 0.5 to sample at pixel centers, use (max - min) not (max - min + 1) to stay within bounds
        float pixel_x = rect.x + rect.pixel_min_x + 0.5f + tx * (rect.pixel_max_x - rect.pixel_min_x);
        float pixel_y = rect.y + rect.pixel_min_y + 0.5f + ty * (rect.pixel_max_y - rect.pixel_min_y);

        // Convert to UV
        float u = pixel_x / (float)adoc->size.x;
        float v = pixel_y / (float)adoc->size.y;

        return {u, v};
    }

    void GetExportQuadGeometry(AtlasDocument* adoc, const AtlasRect& rect,
        Vec2* out_min, Vec2* out_max,
        float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max) {

        float dpi = (float)adoc->dpi;
        float inv_dpi = 1.0f / dpi;

        // Compute raster-aligned bounds (pixel grid aligned)
        // This matches how the rasterizer floors/ceils the bounds
        int raster_min_x = FloorToInt(rect.bounds.min.x * dpi);
        int raster_min_y = FloorToInt(rect.bounds.min.y * dpi);
        int raster_max_x = CeilToInt(rect.bounds.max.x * dpi);
        int raster_max_y = CeilToInt(rect.bounds.max.y * dpi);

        // Raster size in pixels
        int raster_w = raster_max_x - raster_min_x;
        int raster_h = raster_max_y - raster_min_y;

        // Geometry uses raster-aligned bounds so it matches the pixel grid
        *out_min = Vec2{(float)raster_min_x, (float)raster_min_y} * inv_dpi;
        *out_max = Vec2{(float)raster_max_x, (float)raster_max_y} * inv_dpi;

        // UVs map geometry corners to atlas pixel edges (not centers)
        // This ensures the full pixel is sampled across the quad
        float pixel_at_min_x = (float)(rect.x + g_editor.atlas.padding);
        float pixel_at_min_y = (float)(rect.y + g_editor.atlas.padding);
        float pixel_at_max_x = (float)(rect.x + g_editor.atlas.padding + raster_w);
        float pixel_at_max_y = (float)(rect.y + g_editor.atlas.padding + raster_h);

        *out_u_min = pixel_at_min_x / (float)adoc->size.x;
        *out_v_min = pixel_at_min_y / (float)adoc->size.y;
        *out_u_max = pixel_at_max_x / (float)adoc->size.x;
        *out_v_max = pixel_at_max_y / (float)adoc->size.y;
    }

    void GetTightQuadGeometry(AtlasDocument* adoc, const AtlasRect& rect,
        Vec2* out_min, Vec2* out_max,
        float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max) {

        float dpi = (float)adoc->dpi;
        float inv_dpi = 1.0f / dpi;

        // Compute raster-aligned bounds (pixel grid aligned)
        int raster_min_x = FloorToInt(rect.bounds.min.x * dpi);
        int raster_min_y = FloorToInt(rect.bounds.min.y * dpi);
        int raster_max_x = CeilToInt(rect.bounds.max.x * dpi);
        int raster_max_y = CeilToInt(rect.bounds.max.y * dpi);

        // Raster size in pixels
        int raster_w = raster_max_x - raster_min_x;
        int raster_h = raster_max_y - raster_min_y;

        // Geometry uses raster-aligned bounds
        *out_min = Vec2{(float)raster_min_x, (float)raster_min_y} * inv_dpi;
        *out_max = Vec2{(float)raster_max_x, (float)raster_max_y} * inv_dpi;

        // UVs map geometry corners to atlas pixel edges
        float pixel_at_min_x = (float)(rect.x + g_editor.atlas.padding);
        float pixel_at_min_y = (float)(rect.y + g_editor.atlas.padding);
        float pixel_at_max_x = (float)(rect.x + g_editor.atlas.padding + raster_w);
        float pixel_at_max_y = (float)(rect.y + g_editor.atlas.padding + raster_h);

        *out_u_min = pixel_at_min_x / (float)adoc->size.x;
        *out_v_min = pixel_at_min_y / (float)adoc->size.y;
        *out_u_max = pixel_at_max_x / (float)adoc->size.x;
        *out_v_max = pixel_at_max_y / (float)adoc->size.y;
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

    Mesh* GetAtlasOutlineMesh(AtlasDocument* adoc) {
        if (!adoc) return nullptr;

        // Return cached mesh if not dirty and zoom hasn't changed
        if (adoc->outline_mesh && !adoc->outline_dirty && adoc->outline_zoom_version == g_workspace.zoom_version) {
            return adoc->outline_mesh;
        }

        // Free old mesh
        if (adoc->outline_mesh) {
            Free(adoc->outline_mesh);
            adoc->outline_mesh = nullptr;
        }

        // Count valid rects and estimate edge count
        int valid_count = 0;
        int total_edges = 0;
        for (int i = 0; i < adoc->rect_count; i++) {
            if (!adoc->rects[i].valid) continue;
            valid_count++;
            total_edges += 32;  // Conservative estimate for pixel hull edges
        }
        if (valid_count == 0) return nullptr;

        // Each edge is a quad (4 verts, 6 indices)
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, (u16)(total_edges * 4), (u16)(total_edges * 6));

        // Outline thickness - use zoom_ref_scale for consistent width at any zoom
        constexpr float LINE_WIDTH = 0.01f;
        float outline_size = LINE_WIDTH * g_workspace.zoom_ref_scale;

        constexpr float PIXELS_PER_UNIT = 51.2f;
        float pixel_to_world = 1.0f / PIXELS_PER_UNIT;
        Vec4 yellow = {1.0f, 1.0f, 0.0f, 1.0f};

        // Atlas display: centered at origin, Y flipped (atlas pixel Y=0 at display top)
        float half_w = (float)adoc->size.x * 0.5f;
        float half_h = (float)adoc->size.y * 0.5f;

        for (int ri = 0; ri < adoc->rect_count; ri++) {
            const AtlasRect& r = adoc->rects[ri];
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

        adoc->outline_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, adoc->name, true);
        adoc->outline_dirty = false;
        adoc->outline_zoom_version = g_workspace.zoom_version;

        Free(builder);

        return adoc->outline_mesh;
    }

    // Save/Load

    static void SaveAtlasData(Document* doc, const std::filesystem::path& path) {
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

        WriteCSTR(stream, "w %d\n", adoc->size.x);
        WriteCSTR(stream, "h %d\n", adoc->size.y);
        WriteCSTR(stream, "d %d\n", adoc->dpi);
        WriteCSTR(stream, "\n");

        // Save rects: r "asset_name" x y w h frame_count bounds pixel_bounds
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid && adoc->rects[i].asset_name) {
                const AtlasRect& r = adoc->rects[i];
                WriteCSTR(stream, "r \"%s\" %d %d %d %d %d %.6f %.6f %.6f %.6f %d %d %d %d\n",
                    r.asset_name->value,
                    r.x, r.y, r.width, r.height, r.frame_count,
                    r.bounds.min.x, r.bounds.min.y,
                    r.bounds.max.x, r.bounds.max.y,
                    r.pixel_min_x, r.pixel_min_y, r.pixel_max_x, r.pixel_max_y);
            }
        }

        SaveStream(stream, path);
        Free(stream);
    }

    static void ParseRect(AtlasDocument* adoc, Tokenizer& tk) {
        if (!ExpectQuotedString(tk))
            ThrowError("missing rect asset name");

        const Name* asset_name = GetName(tk);

        int x, y, w, h;
        if (!ExpectInt(tk, &x)) ThrowError("missing rect x");
        if (!ExpectInt(tk, &y)) ThrowError("missing rect y");
        if (!ExpectInt(tk, &w)) ThrowError("missing rect width");
        if (!ExpectInt(tk, &h)) ThrowError("missing rect height");

        // frame_count
        int frame_count = 1;
        ExpectInt(tk, &frame_count);

        // bounds (optional for backwards compatibility)
        Bounds2 bounds = BOUNDS2_ZERO;
        float bmin_x = 0.0f;
        float bmin_y = 0.0f;
        float bmax_x = 0.0f;
        float bmax_y = 0.0f;
        bool has_bounds = ExpectFloat(tk, &bmin_x) && ExpectFloat(tk, &bmin_y) &&
                          ExpectFloat(tk, &bmax_x) && ExpectFloat(tk, &bmax_y);
        if (has_bounds) {
            bounds = {{bmin_x, bmin_y}, {bmax_x, bmax_y}};
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

        int slot = FindFreeRectSlot(adoc);
        if (slot >= 0) {
            AtlasRect& rect = adoc->rects[slot];
            rect.asset_name = asset_name;
            rect.x = x;
            rect.y = y;
            rect.width = w;
            rect.height = h;
            rect.frame_count = frame_count;
            rect.bounds = bounds;
            rect.pixel_min_x = pixel_min_x;
            rect.pixel_min_y = pixel_min_y;
            rect.pixel_max_x = pixel_max_x;
            rect.pixel_max_y = pixel_max_y;
            rect.valid = true;

            // Mark this space as used in the packer
            RectPacker::Rect bin_rect(x, y, w, h);
            adoc->packer->MarkUsed(bin_rect);
        }
    }

    static void LoadAtlasData(Document* doc) {
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, doc->path.value);
        Tokenizer tk;
        Init(tk, contents.c_str());

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "w")) {
                adoc->size.x = ExpectInt(tk);
            } else if (ExpectIdentifier(tk, "h")) {
                adoc->size.y = ExpectInt(tk);
            } else if (ExpectIdentifier(tk, "d")) {
                if (!ExpectInt(tk, &adoc->dpi))
                    ThrowError("missing atlas dpi");
            } else if (ExpectIdentifier(tk, "r")) {
                // Ensure packer is initialized with correct dimensions before parsing rects
                if (!adoc->packer) {
                    adoc->packer = new RectPacker(adoc->size.x, adoc->size.y);
                }
                ParseRect(adoc, tk);
            } else {
                char error[256];
                GetString(tk, error, sizeof(error) - 1);
                ThrowError("invalid token '%s' in atlas", error);
            }
        }

        // Initialize packer if no rects were loaded
        if (!adoc->packer) {
            adoc->packer = new RectPacker(adoc->size.x, adoc->size.y);
        }

        // Use same scale as DrawAtlasData (512 pixels = 10 units for grid alignment)
        constexpr float PIXELS_PER_UNIT = 51.2f;
        Vec2 tsize = Vec2{static_cast<float>(adoc->size.x), static_cast<float>(adoc->size.y)} / PIXELS_PER_UNIT;
        doc->bounds = Bounds2{-tsize.x*0.5f, -tsize.y*0.5f, tsize.x*0.5f, tsize.y*0.5f};
    }

    static void PostLoadAtlasData(Document* doc) {
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        // Register as managed atlas if it has the auto-managed naming convention
        RegisterManagedAtlas(adoc);

        // Check for DPI mismatch - regenerate atlas if config DPI differs from saved DPI
        if (adoc->dpi != g_editor.atlas.dpi) {
            LogInfo("Atlas '%s' DPI mismatch (file: %d, config: %d) - regenerating",
                doc->name->value, adoc->dpi, g_editor.atlas.dpi);
            adoc->dpi = g_editor.atlas.dpi;
            RebuildAtlas(adoc);
            MarkModified(doc);
            return;  // RebuildAtlas handles asset binding and rendering
        }

        // Bind sprites to this atlas and render pixels
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid && adoc->rects[i].asset_name) {
                // Check if bounds are valid (non-zero size)
                Vec2 size = GetSize(adoc->rects[i].bounds);
                bool bounds_valid = size.x > F32_EPSILON && size.y > F32_EPSILON;

                SpriteDocument* sdoc = static_cast<SpriteDocument*>(FindDocument(ASSET_TYPE_SPRITE, adoc->rects[i].asset_name));
                if (sdoc) {
                    sdoc->atlas = adoc;
                    RenderSpriteToAtlas(adoc, sdoc, adoc->rects[i], !bounds_valid);
                }
            }
        }
    }

    static void LoadAtlasMetaData(Document* a, Props* meta) {
        AtlasDocument* atlas = static_cast<AtlasDocument*>(a);
        (void)meta;  // Width/height/dpi come from the .atlas file, not metadata

        InitAtlasEditor(atlas);
    }

    static void SaveAtlasMetaData(Document* a, Props* meta) {
        (void)a;
        (void)meta;
        // Width/height/dpi are saved in the .atlas file, not metadata
    }

    void InitAtlasDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ATLAS);

        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);
        adoc->size.x = ATLAS_DEFAULT_SIZE;
        adoc->size.y = ATLAS_DEFAULT_SIZE;
        adoc->dpi = g_editor.atlas.dpi;
        adoc->dirty = true;
        adoc->bounds = Bounds2{Vec2{-5.0f, -5.0f}, Vec2{5.0f, 5.0f}};
        adoc->vtable = {
            .destructor = DestroyAtlasDocument,
            .load = LoadAtlasData,
            .post_load = PostLoadAtlasData,
            .save = SaveAtlasData,
            .load_metadata = LoadAtlasMetaData,
            .save_metadata = SaveAtlasMetaData,
            .draw = DrawAtlasData,
            .clone = CLoneAtlasDocument,
        };
    }

    Document* NewAtlasDocument(const std::filesystem::path& path) {
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

        AtlasDocument* adoc = static_cast<AtlasDocument*>(CreateDocument(full_path));
        LoadAtlasData(adoc);
        return adoc;
    }


    static void ImportAtlas(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);
        TextureFilter filter_value = g_editor.atlas.filter;
        TextureClamp clamp_value = TEXTURE_CLAMP_CLAMP;

        // Regenerate atlas to adoc->pixels (ensures it's up to date)
        RegenerateAtlas(adoc);

        if (!adoc->pixels)
            return;

        u32 pixel_size = adoc->size.x * adoc->size.y * 4;

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, pixel_size + 1024);

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_ATLAS;
        header.version = 1;
        header.flags = ASSET_FLAG_NONE;
        WriteAssetHeader(stream, &header);

        TextureFormat format = TEXTURE_FORMAT_RGBA8;
        WriteU8(stream, (u8)format);
        WriteU8(stream, (u8)filter_value);
        WriteU8(stream, (u8)clamp_value);
        WriteU32(stream, adoc->size.x);
        WriteU32(stream, adoc->size.y);
        WriteBytes(stream, adoc->pixels->rgba, pixel_size);

        SaveStream(stream, path);
        Free(stream);
    }

    static bool CheckAtlasDependency(Document* doc, Document* dependency) {
        if (dependency->def->type != ASSET_TYPE_SPRITE) {
            return false;
        }

        AtlasDocument* adoc = static_cast<AtlasDocument*>(doc);

        // Check if this sprite is attached to the atlas
        for (int i = 0; i < adoc->rect_count; i++) {
            if (adoc->rects[i].valid && adoc->rects[i].asset_name == dependency->name) {
                return true;
            }
        }

        return false;
    }

    void InitAtasDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_ATLAS,
            .size = sizeof(AtlasDocument),
            .ext = ".atlas",
            .init_func = InitAtlasDocument,
            .import_func = ImportAtlas,
            .check_dependency_func = CheckAtlasDependency
        });
    }
}
