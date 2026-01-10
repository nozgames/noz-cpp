//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor::shape {

    constexpr int SHAPE_MAX_ANCHORS = 2048;
    constexpr int SHAPE_MAX_SEGMENTS = 2048;
    constexpr int SHAPE_MAX_PATHS = 1024;
    constexpr int SHAPE_MAX_SEGMENT_SAMPLES = 16;

    struct Anchor {
        Vec2 position;
        bool active;
    };

    struct Segment {
        u16 anchor0;
        u16 anchor1;
        u16 path_left;
        u16 path_right;
        Vec2 samples[SHAPE_MAX_SEGMENT_SAMPLES];

        struct {
            Vec2 offset;
            float weight;
        } curve;

        u16 sample_count;
        bool active;
    };

    struct Path {
        u16 segments[SHAPE_MAX_SEGMENTS];
        u16 segment_count;
        u8 stroke_color;
        u8 fill_color;
    };

    struct Shape {
        Anchor anchors[SHAPE_MAX_ANCHORS];
        Segment segments[SHAPE_MAX_SEGMENTS];
        Path paths[SHAPE_MAX_PATHS];

        u16 anchor_count;
        u16 segment_count;
        u16 path_count;
    };


    extern void UpdateSegmentSamples(Shape* shape, u16 s_idx);
}
