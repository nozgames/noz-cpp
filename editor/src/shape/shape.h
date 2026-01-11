//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor::shape {

    constexpr int SHAPE_MAX_ANCHORS = 1024;
    constexpr int SHAPE_MAX_PATHS = 256;
    constexpr int SHAPE_MAX_SEGMENT_SAMPLES = 8;

    enum AnchorFlags : u16 {
        ANCHOR_FLAG_NONE = 0,
        ANCHOR_FLAG_SELECTED = 1 << 0,
        ANCHOR_FLAG_ALL = U16_MAX
    };

    struct Anchor {
        Vec2 position;
        float curve;
        u16 flags;
        Vec2 samples[SHAPE_MAX_SEGMENT_SAMPLES];
    };

    enum PathFlags : u16 {
        PATH_FLAG_NONE = 0,
        PATH_FLAG_SELECTED = 1 << 0,
        PATH_FLAG_ALL = U16_MAX
    };

    struct Path {
        u16 anchor_start;
        u16 anchor_count;
        u8 stroke_color;
        u8 fill_color;
        PathFlags flags;
    };

    struct Shape {
        Anchor anchors[SHAPE_MAX_ANCHORS];
        Path paths[SHAPE_MAX_PATHS];
        u16 anchor_count;
        u16 path_count;
        Bounds2 bounds;
        RectInt raster_bounds;
    };

    struct HitResult {
        u16 anchor_index = U16_MAX;
        u16 segment_index = U16_MAX;
        u16 path_index = U16_MAX;
    };

    // @shape
    extern void UpdateSamples(Shape* shape);
    extern void UpdateSamples(Shape* shape, u16 path_idx, u16 anchor_idx);
    extern void UpdateBounds(Shape* shape);
    extern bool HitTest(Shape* shape, const Vec2& point, HitResult* result);
    extern u16 HitTestAll(Shape* shape, const Vec2& point, HitResult* results, u16 max_results);
    extern void ClearSelection(Shape* shape);
    extern void Rasterize(Shape* shape, PixelData* pixels, const Color* palette, const Vec2Int& offset);
    
    // @anchor
    inline void SetFlags(Anchor* anchor, AnchorFlags mask, AnchorFlags flags) {
        anchor->flags = (AnchorFlags)(anchor->flags | flags);
    }
    inline void ClearFlags(Anchor* anchor, AnchorFlags mask = ANCHOR_FLAG_ALL) {
        anchor->flags = (AnchorFlags)(anchor->flags & ~mask);
    }
    inline bool HasFlag(const Anchor* anchor, AnchorFlags flag) {
        return (anchor->flags & flag) != 0;
    }
    inline bool IsSelected(const Anchor* anchor) { return HasFlag(anchor, ANCHOR_FLAG_SELECTED); }

    extern void DeleteSelectedAnchors(Shape* shape);

    // @path
    inline void SetFlags(Path* path, PathFlags mask, PathFlags flags) {
        path->flags = (PathFlags)(path->flags | flags);
    }
    inline void ClearFlags(Path* path, PathFlags mask = PATH_FLAG_ALL) {
        path->flags = (PathFlags)(path->flags & ~mask);
    }
    inline bool HasFlag(Path* path, PathFlags flag) {
        return (path->flags & flag) != 0;
    }
    inline bool IsSelected(const Path* path) { return HasFlag((Path*)path, PATH_FLAG_SELECTED); }

    // Helper to get anchor from path
    inline Anchor* GetAnchor(Shape* shape, Path* path, u16 local_idx) {
        return &shape->anchors[path->anchor_start + local_idx];
    }

    inline Anchor* GetAnchor(Shape* shape, const Path* path, u16 local_idx) {
        return &shape->anchors[path->anchor_start + local_idx];
    }

}
