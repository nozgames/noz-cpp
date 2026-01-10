//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "platform.h"

namespace noz {
    void SetThreadName(const char* name) {
        PlatformSetThreadName(name);
    }
}
