cmake_minimum_required(VERSION 3.5)

project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Renderer backend option (Windows only - macOS uses Metal)
set(NOZ_RENDERER "VULKAN" CACHE STRING "Renderer backend (VULKAN or GLES)")
set_property(CACHE NOZ_RENDERER PROPERTY STRINGS "VULKAN" "GLES")

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
    src/editor/editor_client.cpp
    src/editor/editor_messages.cpp
    src/hash.cpp
    src/event.cpp
    src/input/input.cpp
    src/input/input_set.cpp
    src/log.cpp
    src/tween.cpp
    src/job.cpp
    src/http.cpp
    src/websocket.cpp
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

)

# Add platform-specific files
if(EMSCRIPTEN)
    list(APPEND SOURCE_FILES
        src/platform/web/web_main.cpp
        src/platform/web/web_render.cpp
        src/platform/web/web_input.cpp
        src/platform/web/web_audio.cpp
        src/platform/web/web_time.cpp
        src/platform/web/web_http.cpp
        src/platform/web/web_websocket.cpp
        src/platform/gles/gles_render.cpp
    )
elseif(WIN32)
    list(APPEND SOURCE_FILES
        src/platform/windows/windows_main.cpp
        src/platform/windows/windows_input.cpp
        src/platform/windows/windows_audio.cpp
        src/platform/windows/windows_time.cpp
        src/platform/windows/windows_http.cpp
        src/platform/windows/windows_websocket.cpp
    )

    # Add renderer-specific files based on NOZ_RENDERER option
    if(NOZ_RENDERER STREQUAL "GLES")
        list(APPEND SOURCE_FILES
            src/platform/gles/gles_render.cpp
            src/platform/gles/gles_windows.cpp
        )
    else()
        list(APPEND SOURCE_FILES
            src/platform/vulkan/vulkan_render.cpp
            src/platform/vulkan/vulkan_windows.cpp
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
endif()

add_library(noz STATIC ${SOURCE_FILES})

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

# Common include directories
target_include_directories(noz PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform-specific include directories
if(EMSCRIPTEN)
    target_compile_definitions(noz PUBLIC NOZ_PLATFORM_WEB NOZ_PLATFORM_GLES)
    message(STATUS "Building for Web/Emscripten with WebGL")
elseif(WIN32)
    target_include_directories(noz PUBLIC
        ${CMAKE_SOURCE_DIR}/res/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan
    )
    target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

    # Add renderer-specific compile definitions
    if(NOZ_RENDERER STREQUAL "GLES")
        target_compile_definitions(noz PUBLIC NOZ_PLATFORM_GLES)
        message(STATUS "Using GLES renderer backend")
    else()
        target_compile_definitions(noz PUBLIC NOZ_PLATFORM_VULKAN)
        message(STATUS "Using Vulkan renderer backend")
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
            -sINITIAL_MEMORY=33554432
            -lwebsocket.js
    )
    # Box2D and enet would need to be compiled with emscripten separately
    message(STATUS "Emscripten build - external libraries will need emscripten-compiled versions")
elseif(WIN32)
    target_link_libraries(noz
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/box2d.lib
            ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/enet.lib
            ws2_32
            winmm
            dwmapi
            Shcore
            xaudio2
            ole32
    )
elseif(APPLE)
    target_link_libraries(noz
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/libbox2d.a
            ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/libenet.a
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
