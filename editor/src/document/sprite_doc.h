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

    struct SpriteDataImpl {
        SpriteFrame frames[SPRITE_MAX_FRAMES];
        u16 frame_count;
    };

    struct SpriteDocument : Document {
        SpriteDataImpl* impl;
    };

    extern void InitSpriteData(Document* a);
}
