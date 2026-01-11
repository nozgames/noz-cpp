//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "../shape/shape.h"

namespace noz::editor {

    constexpr int SPRITE_MAX_FRAMES = 64;

    struct SpriteFrame {
        shape::Shape shape;
        int hold;
    };

    struct SpriteDocument : Document {
        SpriteFrame frames[SPRITE_MAX_FRAMES];
        u16 frame_count;
        u8 palette;
    };

    extern void InitSpriteData(Document* a);
    inline SpriteFrame* GetFrame(SpriteDocument* sprite, u16 frame_index) {
        return &sprite->frames[frame_index];
    }
}
