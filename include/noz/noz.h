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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

using namespace glm;

#include "noz_math.h"

#define VEC3_FORWARD vec3(0, 0, 1)
#define VEC3_BACKWARD vec3(0, 0, -1)
#define VEC3_UP vec3(0, 1, 0)
#define VEC3_DOWN vec3(0, -1, 0)
#define VEC3_RIGHT vec3(1, 0, 0)
#define VEC3_LEFT vec3(-1, 0, 0)
#define VEC3_ZERO vec3(0,0,0)
#define VEC3_ONE vec3(1,1,1)
#define VEC4_ZERO vec4(0,0,0,0)
#define VEC4_ONE vec4(1,1,1,1)
#define VEC2_ZERO vec2(0,0)
#define VEC2_ONE vec2(1,1)

inline int i32_max(i32 a, i32 b) { return (a > b) ? a : b; }
inline int i32_min(i32 a, i32 b) { return (a < b) ? a : b; }

#include "allocator.h"
#include "object.h"
#include "collections.h"
#include "log.h"
#include "bounds3.h"
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
#include "entity.h"
#include "types.h"
#include "ui.h"
#include "input.h"
#include "physics.h"
#include "core_assets.h"
#include "editor.h"
#include "vfx.h"
