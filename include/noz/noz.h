//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <functional>
#include <filesystem>
#include <mutex>

#include "noz_math.h"
#include "string.h"
#include "allocator.h"
#include "collections.h"
#include "log.h"
#include "event.h"
#include "name.h"
#include "color.h"
#include "hash.h"
#include "stream.h"
#include "asset.h"
#include "renderer.h"
#include "animation.h"
#include "application.h"
#include "ui.h"
#include "input.h"
#include "physics.h"
#include "core_assets.h"
#include "vfx.h"
#include "audio.h"
#include "http.h"
#include "websocket.h"
#include "prefs.h"
#include "core_events.h"
#include "tween.h"
#include "debug.h"
#include "rect.h"
#include "bin.h"

#if defined(NOZ_LUA)
#include "noz_lua.h"
#endif
