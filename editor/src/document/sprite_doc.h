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
        float depth;
        struct {
            SkeletonDocument* skeleton;
            const Name* skeleton_name;
            u8 bone;
        } skin;

        // Atlas integration
        struct AtlasDocument* atlas;
        bool atlas_dirty;
    };

    extern void InitSpriteData(Document* a);
    extern void UpdateBounds(SpriteDocument* sdoc);
    inline SpriteFrame* GetFrame(SpriteDocument* sprite, u16 frame_index) {
        return &sprite->frames[frame_index];
    }

    inline bool HasSkin(SpriteDocument* sdoc) {
        if (!sdoc) return false;
        return sdoc->skin.skeleton_name && sdoc->skin.bone != U8_MAX;
    }
}
