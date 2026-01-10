//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
// @STL

#include "atlas_manager.h"

namespace noz::editor {
    namespace fs = std::filesystem;
    using namespace noz;

    constexpr Vec2 OUTLINE_COLOR = ColorUV(0, 10);

    struct OutlineConfig {
        float width;
        float offset;
        float boundary_taper;
    };

}
