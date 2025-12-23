cmake_minimum_required(VERSION 3.5)

project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Renderer backend option (Windows only - macOS uses Metal)
set(NOZ_RENDERER "GL" CACHE STRING "Renderer backend (VULKAN, GLES, or GL)")
set_property(CACHE NOZ_RENDERER PROPERTY STRINGS "VULKAN" "GLES" "GL")

option(NOZ_HTTP "Enable HTTP support" OFF)
option(NOZ_WEBSOCKET "Enable WebSocket support" OFF)
option(NOZ_FETCH_ZLIB "Fetch zlib (disable if already provided by parent project)" ON)

# Fetch zlib for gzip decompression in WebSocket messages
if(NOZ_WEBSOCKET AND NOZ_FETCH_ZLIB)
    message(STATUS "Fetching zlib for WebSocket gzip decompression...")

    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
endif()


# Explicitly list all source files
set(SOURCE_FILES
    src/application.cpp
    src/asset.cpp
    src/string.cpp
    src/thread.cpp
    src/tokenizer.cpp
    src/collections/list.cpp
    src/collections/linked_list.cpp
    src/collections/map.cpp
    src/collections/ring_buffer.cpp
    src/collections/free_list.cpp
    src/color.cpp
    src/hash.cpp
    src/event.cpp
    src/input/input.cpp
    src/input/input_set.cpp
    src/log.cpp
    src/bin.cpp
    src/tween.cpp
    src/job.cpp
    src/task.cpp
    src/memory/allocator.cpp
    src/memory/pool_allocator.cpp
    src/memory/arena_allocator.cpp
    src/math/math.cpp
    src/math/transform.cpp
    src/math/perlin.cpp
    src/math/bounds2.cpp
    src/math/bounds3.cpp
    src/math/easing.cpp
    src/math/mat3.cpp
    src/math/mat4.cpp
    src/math/vec2.cpp
    src/math/vec3.cpp
    src/math/angle.cpp
    src/name.cpp
    src/debug.cpp
    src/physics/physics.cpp
    src/physics/rigid_body.cpp
    src/physics/collider.cpp
    src/physics/collision.cpp
    src/random.cpp
    src/rect.cpp
    src/render/camera.cpp
    src/render/material.cpp
    src/render/mesh_builder.cpp
    src/render/mesh.cpp
    src/render/font.cpp
    src/render/texture.cpp
    src/render/shader.cpp
    src/render/render_buffer.cpp
    src/render/renderer.cpp
    src/render/skeleton.cpp
    src/render/animation.cpp
    src/render/animator.cpp
    src/render/blend_tree.cpp
    src/render/animated_mesh.cpp
    src/stream.cpp
    src/time.cpp
    src/text.cpp
    src/ui/ui.cpp
    src/ui/text_engine.cpp
    src/vfx/vfx.cpp
    src/vfx/vfx_system.cpp
    src/audio/audio.cpp
    src/audio/sound.cpp
    src/prefs.cpp
    src/http.cpp
    src/websocket.cpp
)

# Add platform-specific files
if(EMSCRIPTEN)
    list(APPEND SOURCE_FILES
        src/platform/web/web_main.cpp
        src/platform/gl/gl_web.cpp
        src/platform/web/web_input.cpp
        src/platform/web/web_audio.cpp
        src/platform/web/web_time.cpp
        src/platform/gl/gl_render.cpp
    )

    # WebSocket backend selection for web
    if(NOZ_WEBSOCKET)
        list(APPEND SOURCE_FILES src/platform/web/web_websocket.cpp)
    else()
        list(APPEND SOURCE_FILES src/platform/null/null_websocket.cpp)
    endif()

    if(NOZ_HTTP)
        list(APPEND SOURCE_FILES src/platform/web/web_http.cpp)
    else()
        list(APPEND SOURCE_FILES src/platform/null/null_http.cpp)
    endif()
elseif(WIN32)
    list(APPEND SOURCE_FILES
        src/platform/windows/windows_main.cpp
        src/platform/windows/windows_input.cpp
        src/platform/windows/windows_audio.cpp
        src/platform/windows/windows_time.cpp
    )

    # WebSocket backend selection for Windows
    if(NOZ_WEBSOCKET)
        list(APPEND SOURCE_FILES src/platform/windows/windows_websocket.cpp)
    else()
        list(APPEND SOURCE_FILES src/platform/null/null_websocket.cpp)
    endif()

    # HTTP backend selection for Windows
    if(NOZ_HTTP)
        list(APPEND SOURCE_FILES src/platform/windows/windows_http.cpp)
    else()
        list(APPEND SOURCE_FILES src/platform/null/null_http.cpp)
    endif()

    # Add renderer-specific files based on NOZ_RENDERER option
    if(NOZ_RENDERER STREQUAL "GLES" OR NOZ_RENDERER STREQUAL "GL")
        list(APPEND SOURCE_FILES
            src/platform/gl/gl_render.cpp
            src/platform/gl/gl_windows.cpp
        )
    else()
        list(APPEND SOURCE_FILES
            src/platform/vulkan/vulkan_init.cpp
            src/platform/vulkan/vulkan_proc.cpp
            src/platform/vulkan/vulkan_render.cpp
            src/platform/vulkan/vulkan_windows.cpp
            src/platform/vulkan/vulkan_util.cpp
        )
    endif()
elseif(APPLE)
    list(APPEND SOURCE_FILES
        src/platform/macosx/macosx_main.cpp
        src/platform/macosx/macosx_input.cpp
        src/platform/macosx/macosx_audio.cpp
        src/platform/macosx/macosx_time.cpp
        src/platform/macosx/macosx_render.cpp
        src/platform/macosx/macosx_metal.cpp
    )
    # Enable Objective-C++ for macOS files
    set_source_files_properties(
        src/platform/macosx/macosx_main.cpp
        src/platform/macosx/macosx_input.cpp
        src/platform/macosx/macosx_audio.cpp
        src/platform/macosx/macosx_render.cpp
        src/platform/macosx/macosx_metal.cpp
        PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )

    # WebSocket backend selection for macOS (null stub for now)
    list(APPEND SOURCE_FILES src/platform/null/null_websocket.cpp)

    # HTTP backend selection for macOS (null stub for now)
    list(APPEND SOURCE_FILES src/platform/null/null_http.cpp)
endif()

add_library(noz STATIC ${SOURCE_FILES})


# Add WebSocket compile definition if enabled
if(NOZ_WEBSOCKET)
    target_compile_definitions(noz PUBLIC NOZ_WEBSOCKET)
endif()

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

# Common include directories
target_include_directories(noz PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform-specific include directories
if(EMSCRIPTEN)
    target_compile_definitions(noz PUBLIC NOZ_PLATFORM_WEB NOZ_PLATFORM_GLES NOZ_JOB_THREAD_COUNT=4)
    target_compile_options(noz PUBLIC -pthread)
    if(NOZ_HTTP)
        target_compile_definitions(noz PUBLIC NOZ_HTTP)
    endif()
    message(STATUS "Building for Web/Emscripten with WebGL and pthreads")
elseif(WIN32)
    target_include_directories(noz PUBLIC
        ${CMAKE_SOURCE_DIR}/res/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan
    )
    target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

    # Add renderer-specific compile definitions
    if(NOZ_RENDERER STREQUAL "GL")
        target_compile_definitions(noz PUBLIC NOZ_PLATFORM_GL)
    else()
        target_compile_definitions(noz PUBLIC NOZ_PLATFORM_VULKAN)
    endif()
elseif(APPLE)
    # macOS uses system frameworks, no additional include paths needed
endif()

# Link dependencies for noz
if(EMSCRIPTEN)
    # Emscripten link flags for web APIs
    target_link_options(noz
        PUBLIC
            -sUSE_WEBGL2=1
            -sFULL_ES3=1
            -sFETCH=1
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=128MB
            -pthread
            -sPTHREAD_POOL_SIZE=5
    )
    if(NOZ_WEBSOCKET)
        target_compile_options(noz PUBLIC -sUSE_ZLIB=1)
        target_link_options(noz PUBLIC -lwebsocket.js -sUSE_ZLIB=1)
    endif()
    # Box2D and enet would need to be compiled with emscripten separately
    message(STATUS "Emscripten build - external libraries will need emscripten-compiled versions")
elseif(WIN32)
    target_link_libraries(noz
        PUBLIC
            winmm
            dwmapi
            Shcore
            xaudio2
            ole32
    )

    # Link HTTP backend libraries
    if(NOZ_HTTP)
        target_link_libraries(noz PUBLIC winhttp)
        target_compile_definitions(noz PUBLIC NOZ_HTTP)
    endif()
    # Link WebSocket libraries
    if(NOZ_WEBSOCKET)
        if(NOT NOZ_HTTP)
            # Link winhttp for WebSocket (if not already linked by HTTP)
            target_link_libraries(noz PUBLIC winhttp)
        endif()
        # Link zlib for gzip decompression
        target_link_libraries(noz PUBLIC zlibstatic)
    endif()
elseif(APPLE)
    target_link_libraries(noz
        PUBLIC
            "-framework Cocoa"
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
            "-framework GameController"
            "-framework AudioToolbox"
            "-framework Carbon"
    )
endif()

if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # This makes ninja show shorter status messages
    set(ENV{NINJA_STATUS} "[%f/%t] ")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")
endif()
