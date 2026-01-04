//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz { class rect_packer; }

constexpr int ATLAS_MAX_RECTS = 256;
constexpr int ATLAS_DEFAULT_DPI = 96;
constexpr int ATLAS_DEFAULT_SIZE = 1024;

struct AtlasRect {
    int x, y, width, height;
    const Name* mesh_name;   // Which mesh owns this rect
    Bounds2 mesh_bounds;     // Cached bounds for change detection
    bool valid;              // Is this rect in use?
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
    noz::rect_packer* packer = nullptr;
};

struct AtlasData : AssetData {
    AtlasDataImpl* impl;
};

// Lifecycle
extern void InitAtlasData(AssetData* a);
extern AssetData* NewAtlasData(const std::filesystem::path& path);

// Rect management
extern AtlasRect* AllocateRect(AtlasData* atlas, const Name* mesh_name, const Bounds2& bounds);
extern AtlasRect* FindRectForMesh(AtlasData* atlas, const Name* mesh_name);
extern void FreeRect(AtlasData* atlas, AtlasRect* rect);
extern void ClearAllRects(AtlasData* atlas);

// Rendering
extern void RenderMeshToAtlas(AtlasData* atlas, struct MeshData* mesh, const AtlasRect& rect);
extern void UpdateAtlas(AtlasData* atlas);
extern void RegenerateAtlas(AtlasData* atlas);
extern void SyncAtlasTexture(AtlasData* atlas);  // Upload pixels to GPU (editor only)

// UV computation
extern Vec2 GetAtlasUV(AtlasData* atlas, const AtlasRect& rect, const Vec2& position);

// Importer
struct AssetImporter;
extern AssetImporter GetAtlasImporter();
