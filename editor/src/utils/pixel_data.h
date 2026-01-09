//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct PixelData {
    Color32* rgba;
    Vec2Int size;
};

extern PixelData* CreatePixelData(Allocator* allocator, const Vec2Int& size);
extern void Clear(PixelData* p, const Color32& color = COLOR32_TRANSPARENT);
extern void Set(PixelData* p, const noz::RectInt& rect, const Color32& color);
