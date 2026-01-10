//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct TextureDocument : Document {
        Texture* texture;
        float scale;
    };

    extern void UpdateBounds(TextureDocument* doc);
}

