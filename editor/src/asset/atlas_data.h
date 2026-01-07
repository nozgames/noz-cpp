//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz { class RectPacker; }

constexpr int ATLAS_MAX_RECTS = 256;
constexpr int ATLAS_DEFAULT_DPI = 96;
constexpr int ATLAS_DEFAULT_SIZE = 1024;
constexpr int ATLAS_RECT_PADDING = 2;  // Padding on each side of content in rect

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

struct AtlasDataImpl {
    int width = ATLAS_DEFAULT_SIZE;
    int height = ATLAS_DEFAULT_SIZE;
    int dpi = ATLAS_DEFAULT_DPI;
    AtlasRect rects[ATLAS_MAX_RECTS];
    int rect_count = 0;
    u8* pixels = nullptr;
    bool dirty = true;
    Texture* texture = nullptr;
    Material* material = nullptr;
    noz::RectPacker* packer = nullptr;
};

struct AtlasData : AssetData {
    AtlasDataImpl* impl;
};

// Lifecycle
extern void InitAtlasData(AssetData* a);
extern AssetData* NewAtlasData(const std::filesystem::path& path);

// Rect management
extern AtlasRect* AllocateRect(AtlasData* atlas, struct MeshData* mesh);  // Allocates all frames for multi-frame meshes
extern AtlasRect* FindRectForMesh(AtlasData* atlas, const Name* mesh_name);
extern void FreeRect(AtlasData* atlas, AtlasRect* rect);
extern void ClearRectPixels(AtlasData* atlas, const AtlasRect& rect);
extern void ClearAllRects(AtlasData* atlas);

// Find the atlas containing a mesh (searches all atlases)
extern AtlasData* FindAtlasForMesh(const Name* mesh_name, AtlasRect** out_rect = nullptr);

// Rendering
extern void RenderMeshToAtlas(AtlasData* atlas, struct MeshData* mesh, AtlasRect& rect, bool update_bounds = true);  // Renders all frames
extern void RegenerateAtlas(AtlasData* atlas, u8* buffer = nullptr);  // Re-render meshes to buffer (or impl->pixels if null)
extern void DilateAtlasRect(u8* pixels, int atlas_width, int atlas_height, const AtlasRect& rect);  // Dilate rect edges for bilinear filtering
extern void RebuildAtlas(AtlasData* atlas);      // Clear and reallocate all rects, mark meshes modified
extern void SyncAtlasTexture(AtlasData* atlas);  // Upload pixels to GPU (editor only)

// UV computation
extern Vec2 GetAtlasUV(AtlasData* atlas, const AtlasRect& rect, const Bounds2& mesh_bounds, const Vec2& position);

// Importer
struct AssetImporter;
extern AssetImporter GetAtlasImporter();
