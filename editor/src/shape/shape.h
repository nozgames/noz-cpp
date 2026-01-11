//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor::shape {

    constexpr int SHAPE_MAX_ANCHORS = 2048;
    constexpr int SHAPE_MAX_SEGMENTS = 2048;
    constexpr int SHAPE_MAX_PATHS = 1024;
    constexpr int SHAPE_MAX_SEGMENT_SAMPLES = 16;

    enum AnchorFlags : u16 {
        ANCHOR_FLAG_NONE = 0,
        ANCHOR_FLAG_ACTIVE = 1 << 0,
        ANCHOR_FLAG_SELECTED = 1 << 1,
        ANCHOR_FLAG_ALL = U16_MAX
    };

    struct Anchor {
        Vec2 position;
        u16 flags;
    };

    enum SegmentFlags : u16 {
        SEGMENT_FLAG_NONE = 0,
        SEGMENT_FLAG_ACTIVE = 1 << 0,
        SEGMENT_FLAG_SELECTED = 1 << 1,
        SEGMENT_FLAG_ALL = U16_MAX
    };

    struct Segment {
        u16 anchor0;
        u16 anchor1;
        u16 path_left;
        u16 path_right;
        u16 sample_count;
        SegmentFlags flags;
        Vec2 samples[SHAPE_MAX_SEGMENT_SAMPLES];

        struct {
            Vec2 offset;
            float weight;
        } curve;
    };

    enum PathFlags : u16 {
        PATH_FLAG_NONE = 0,
        PATH_FLAG_ACTIVE = 1 << 0,
        PATH_FLAG_SELECTED = 1 << 1,
        PATH_FLAG_CLOSED = 1 << 2,

        PATH_FLAG_ALL = U16_MAX
    };

    struct Path {
        u16 segments[SHAPE_MAX_SEGMENTS];
        u16 segment_count;
        u8 stroke_color;
        u8 fill_color;
        PathFlags flags;
    };

    struct Shape {
        Anchor anchors[SHAPE_MAX_ANCHORS];
        Segment segments[SHAPE_MAX_SEGMENTS];
        Path paths[SHAPE_MAX_PATHS];

        u16 anchor_count;
        u16 segment_count;
        u16 path_count;
    };

    struct HitResult {
        u16 anchor_index = U16_MAX;
        u16 segment_index = U16_MAX;
        u16 path_index = U16_MAX;
    };

    // @shape
    extern void UpdateSegmentSamples(Shape* shape, u16 s_idx);
    extern void UpdateSegmentPaths(Shape* shape);
    extern bool HitTest(Shape* shape, const Vec2& point, HitResult* result);
    extern void ClearSelection(Shape* shape);

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
    inline bool IsActive(const Anchor* anchor) { return HasFlag(anchor, ANCHOR_FLAG_ACTIVE); }
    inline bool IsSelected(const Anchor* anchor) { return HasFlag(anchor, ANCHOR_FLAG_SELECTED); }

    // @segment
    inline void SetFlags(Segment* seg, SegmentFlags mask, SegmentFlags flags) {
        seg->flags = (SegmentFlags)(seg->flags | flags);
    }
    inline void ClearFlags(Segment* seg, SegmentFlags mask = SEGMENT_FLAG_ALL) {
        seg->flags = (SegmentFlags)(seg->flags & ~mask);
    }
    inline bool HasFlag(const Segment* seg, SegmentFlags flag) {
        return (seg->flags & flag) != 0;
    }
    inline bool IsActive(const Segment* seg) { return HasFlag(seg, SEGMENT_FLAG_ACTIVE); }
    inline bool IsSelected(const Segment* seg) { return HasFlag(seg, SEGMENT_FLAG_SELECTED); }

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
    inline bool IsActive(const Path* path) { return HasFlag((Path*)path, PATH_FLAG_ACTIVE); }
    inline bool IsSelected(const Path* path) { return HasFlag((Path*)path, PATH_FLAG_SELECTED); }
}
