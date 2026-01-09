//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz { class RectPacker; }

namespace noz::editor {
    struct PixelData;

    constexpr int ATLAS_MAX_RECTS = 256;
    constexpr int ATLAS_DEFAULT_DPI = 96;
    constexpr int ATLAS_DEFAULT_SIZE = 1024;
    constexpr int ATLAS_RECT_PADDING = 4;  // Padding on each side of content in rect

    struct AtlasRect {
        int x, y, width, height;         // Full rect (for animated: width = frame_width * frame_count)
        const Name* mesh_name;           // Which mesh/animated mesh owns this rect
        bool valid;                      // Is this rect in use?
        int frame_count;                 // Number of frames (1 = static mesh, >1 = animated strip)
        Bounds2 mesh_bounds;             // Mesh bounds in world space (for UV calculation)
        int pixel_min_x, pixel_min_y;    // Actual rendered content bounds (from pixel scan)
        int pixel_max_x, pixel_max_y;    // Relative to rect origin
        // For animated: frame_width = width / frame_count, frames are laid out left-to-right
    };

    struct AtlasDocument : Document {
        Vec2Int size = {ATLAS_DEFAULT_SIZE, ATLAS_DEFAULT_SIZE};
        int dpi = ATLAS_DEFAULT_DPI;
        AtlasRect rects[ATLAS_MAX_RECTS];
        int rect_count = 0;
        PixelData* pixels = nullptr;
        bool dirty = true;
        Texture* texture = nullptr;
        Material* material = nullptr;
        noz::RectPacker* packer = nullptr;
        Mesh* outline_mesh = nullptr;       // Editor outline mesh for all export quads
        bool outline_dirty = true;          // Rebuild outline mesh when true
        int outline_zoom_version = 0;       // Track zoom for outline rebuild
    };

    // Lifecycle
    extern void InitAtlasDocument(Document* doc);
    extern Document* NewAtlasDocument(const std::filesystem::path& path);

    // Rect management
    extern AtlasRect* AllocateRect(AtlasDocument* atlas, struct MeshDocument* mesh);  // Allocates all frames for multi-frame meshes
    extern AtlasRect* FindRectForMesh(AtlasDocument* atlas, const Name* mesh_name);
    extern void FreeRect(AtlasDocument* atlas, AtlasRect* rect);
    extern void ClearRectPixels(AtlasDocument* atlas, const AtlasRect& rect);
    extern void ClearAllRects(AtlasDocument* atlas);

    // Find the atlas containing a mesh (searches all atlases)
    extern AtlasDocument* FindAtlasForMesh(const Name* mesh_name, AtlasRect** out_rect = nullptr);

    // Rendering
    extern void RenderMeshToAtlas(AtlasDocument* atlas, MeshDocument* mesh, AtlasRect& rect, bool update_bounds = true);  // Renders all frames
    extern void RenderMeshPreview(MeshDocument* mesh, PixelData* pixels, int width, int height, const Bounds2* bounds);  // Render current frame to buffer
    extern void RegenerateAtlas(AtlasDocument* atlas, PixelData* pixels = nullptr);  // Re-render meshes to buffer (or impl->pixels if null)
    extern void RebuildAtlas(AtlasDocument* atlas);      // Clear and reallocate all rects, mark meshes modified
    extern void SyncAtlasTexture(AtlasDocument* atlas);  // Upload pixels to GPU (editor only)

    // UV computation
    extern Vec2 GetAtlasUV(AtlasDocument* atlas, const AtlasRect& rect, const Bounds2& mesh_bounds, const Vec2& position);

    // Export mesh visualization (single outline mesh for all rects)
    extern Mesh* GetAtlasOutlineMesh(AtlasDocument* atlas);

    // Export quad geometry calculation (shared between importer and visualization)
    // Returns mesh_bounds and UV coordinates for non-skinned mesh export quad (for tiling)
    extern void GetExportQuadGeometry(AtlasDocument* atlas, const AtlasRect& rect,
        Vec2* out_min, Vec2* out_max,
        float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max);

    // Returns tight pixel bounds and UV coordinates for non-skinned mesh export quad
    extern void GetTightQuadGeometry(AtlasDocument* atlas, const AtlasRect& rect,
        Vec2* out_min, Vec2* out_max,
        float* out_u_min, float* out_v_min, float* out_u_max, float* out_v_max);

    // Skinned mesh detection
    extern bool IsSkinnedMesh(struct MeshDocument* mesh);
    extern int GetSingleBoneIndex(struct MeshDocument* mesh);  // Returns bone index or -1 if not single-bone

    // Importer
    struct DocumentImporter;
    extern DocumentImporter GetAtlasImporter();
}
