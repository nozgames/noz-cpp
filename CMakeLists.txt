cmake_minimum_required(VERSION 3.15)
project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

add_library(noz STATIC include/noz/noz_math.h)

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

# Explicitly list all source files
set(SOURCE_FILES
    src/application.cpp
    src/bounds3.cpp
    src/asset.cpp
    src/collections/list.cpp
    src/collections/linked_list.cpp
    src/collections/map.cpp
    src/collections/ring_buffer.cpp
    src/color.cpp
    src/editor/editor_client.cpp
    src/editor/editor_messages.cpp
    src/entity/entity.cpp
    src/entity/camera.cpp
    src/entity/component.cpp
    src/entity/mesh_renderer.cpp
    src/hash.cpp
    src/event.cpp
    src/input/input.cpp
    src/input/input_set.cpp
    src/input/input_code.cpp
    src/log.cpp
    src/memory/allocator.cpp
    src/memory/pool_allocator.cpp
    src/memory/arena_allocator.cpp
    src/math/mat3.cpp
    src/math/mat4.cpp
    src/math/vec2.cpp
    src/name.cpp
    src/object.cpp
    src/physics/physics.cpp
    src/physics/rigid_body.cpp
    src/physics/collider.cpp
    src/random.cpp
    src/rect.cpp
    src/render/material.cpp
    src/render/mesh_builder.cpp
    src/render/pipeline_factory.cpp
    src/render/mesh.cpp
    src/render/font.cpp
    src/render/texture.cpp
    src/render/sampler_factory.cpp
    src/render/shader.cpp
    src/render/render_buffer.cpp
    src/render/renderer.cpp
    src/stream.cpp
    src/type.cpp
    src/time.cpp
    src/text.cpp
    src/ui/canvas.cpp
    src/ui/element.cpp
    src/ui/ui.cpp
    src/ui/style_sheet.cpp
    src/ui/text_engine.cpp
    src/ui/style.cpp
    src/vfx/vfx.cpp
    src/vfx/vfx_system.cpp
)

# Add platform-specific files
if(WIN32)
    list(APPEND SOURCE_FILES src/platform_win.cpp)
else()
    list(APPEND SOURCE_FILES src/platform_posix.cpp)
endif()

target_sources(noz PRIVATE ${SOURCE_FILES})
target_include_directories(noz PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS GLM_ENABLE_EXPERIMENTAL)

# Link dependencies for noz
target_link_libraries(noz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/box2d.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/enet.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl/SDL3-static.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm/glm.lib
        ws2_32
        winmm
        setupapi      # For SetupDi* functions (SDL3)
        cfgmgr32      # For CM_* functions (SDL3)
        version       # For GetFileVersionInfo* functions (SDL3)
        imm32         # For Imm* functions (SDL3 IME support)
        dxcompiler    # For DxcCreateInstance (SDL3_shadercross)
)

add_subdirectory(editor)


if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # This makes ninja show shorter status messages
    set(ENV{NINJA_STATUS} "[%f/%t] ")
endif()

