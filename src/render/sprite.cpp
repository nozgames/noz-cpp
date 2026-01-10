//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

using namespace noz;

Sprite** SPRITE = nullptr;
int SPRITE_COUNT = 0;

namespace noz {

    struct SpriteImpl : Sprite {
    };

    Asset* LoadSprite(
        Allocator* allocator,
        Stream* stream,
        AssetHeader* header,
        const Name* name,
        const Name** name_table) {

        SpriteImpl* impl = static_cast<SpriteImpl*>(Alloc(allocator, sizeof(SpriteImpl), nullptr));
        impl->name = name;
        return impl;
    }
}
