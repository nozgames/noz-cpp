//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct TextureDataImpl {
    Texture* texture;
    Material* material;
    float scale;
};

struct TextureData : AssetData {
    TextureDataImpl* impl;
};

extern void InitTextureData(AssetData* a);
extern void UpdateBounds(TextureData* t);
