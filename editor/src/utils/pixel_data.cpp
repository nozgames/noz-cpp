//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "pixel_data.h"

namespace noz::editor {

    PixelData* CreatePixelData(Allocator* allocator, const Vec2Int& size) {
        PixelData* p = static_cast<PixelData*>(Alloc(allocator, sizeof(PixelData) + size.x * size.y * sizeof(Color32)));
        p->rgba = reinterpret_cast<Color32*>(p + 1);
        p->size = size;
        return p;
    }

    void Clear(PixelData* p, const Color32& color) {
        int total = p->size.x * p->size.y;
        for (int i = 0; i < total; i++)
            p->rgba[i] = color;
    }

    void Set(PixelData* p, const noz::RectInt& rect, const Color32& color) {
        int xe = rect.x + rect.w;
        int ye = rect.y + rect.h;
        for (int y = rect.y; y < ye; y++) {
            for (int x = rect.x; x < xe; x++) {
                p->rgba[y * p->size.x + x] = color;
            }
        }
    }
}

