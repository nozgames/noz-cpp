//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef _DEBUG
#define NOZ_EDITOR
#endif

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "noz_math.h"
#include "allocator.h"
#include "collections.h"
#include "log.h"
#include "rect.h"
#include "event.h"
#include "name.h"
#include "color.h"
#include "text.h"
#include "hash.h"
#include "stream.h"
#include "asset.h"
#include "platform.h"
#include "renderer.h"
#include "application.h"
#include "ui.h"
#include "input.h"
#include "physics.h"
#include "core_assets.h"
#include "vfx.h"
#include "audio.h"
#include "prefs.h"
#include "core_events.h"

#ifdef NOZ_EDITOR
#include "editor.h"
#endif

