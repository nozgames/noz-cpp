//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "shape.h"

namespace noz::editor::shape {

    void UpdateSegmentSamples(Shape* shape, u16 s_idx) {
        Segment* s = &shape->segments[s_idx];
        if (LengthSqr(s->curve.offset) < FLT_EPSILON) {
            s->sample_count = 0;
            return;
        }
        
        Vec2 p0 = shape->anchors[s->anchor0].position;
        Vec2 p1 = shape->anchors[s->anchor1].position;
        Vec2 cp = (p0 + p1) * 0.5f + s->curve.offset;
        float w = s->curve.weight;
        const int num_samples = SHAPE_MAX_SEGMENT_SAMPLES;
        for (int i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i + 1) / static_cast<float>(num_samples + 1);
            float u = 1.0f - t;
            Vec2 point = (p0 * (u * u)) + (cp * (2.0f * u * t)) + (p1 * (t * t));
            s->samples[i] = point;
        }

        s->sample_count = num_samples;
    }
}