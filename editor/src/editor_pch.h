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
#include <stack>
#include <cctype>
#include <cstdio>
#include <cstdarg>
#include <ranges>
#include <fstream>
#include <iostream>
#include <vector>

// Define Windows header control macros before any Windows includes
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN  
#define WIN32_LEAN_AND_MEAN
#endif
#define NOGDI
#define NOUSER

#include <noz/noz.h>
#include "../../noz/src/internal.h"

struct EditorEventStats {
    i32 fps;
};

enum EditorEvent {
    EDITOR_EVENT_STATS,
    EDITOR_EVENT_IMPORTED
};

#include "utils/props.h"
#include "utils/curve_utils.h"
#include <noz/tokenizer.h>
#include "utils/file_helpers.h"
#include "style.h"
#include "editor.h"
#include "document_def.h"

#if !defined(NOZ_EDITOR_LIB)
#include "editor_assets.h"
#endif

#include "editor_mesh_builder.h"

extern Props* g_config;
