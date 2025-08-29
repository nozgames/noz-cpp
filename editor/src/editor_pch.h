//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// Standard Library Headers (commonly used across editor)
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>

#include <noz/noz.h>
#include "tokenizer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
#include "../../src/internal.h"

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <cctype>

#include "asset_importer.h"

static constexpr int TERM_COLOR_STATUS_BAR = 1;
static constexpr int TERM_COLOR_COMMAND_LINE = 2;
static constexpr int TERM_COLOR_SUCCESS = 3;
static constexpr int TERM_COLOR_ERROR = 4;
static constexpr int TERM_COLOR_WARNING = 5;

// Terminal key constants
static constexpr int ERR = -1;
static constexpr int KEY_MOUSE = 409;
