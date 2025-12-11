cmake_minimum_required(VERSION 3.5)

project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Renderer backend option (Windows only - macOS uses Metal)
set(NOZ_RENDERER "VULKAN" CACHE STRING "Renderer backend (VULKAN, GLES, or GL)")
set_property(CACHE NOZ_RENDERER PROPERTY STRINGS "VULKAN" "GLES" "GL")

# HTTP backend option (CURL, PLATFORM, or empty to disable)
set(NOZ_HTTP "" CACHE STRING "HTTP backend (CURL, PLATFORM, or empty to disable)")
set_property(CACHE NOZ_HTTP PROPERTY STRINGS "" "PLATFORM" "CURL")

# WebSocket option (ON or OFF)
option(NOZ_WEBSOCKET "Enable WebSocket support" OFF)

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

# Add WebSocket source files if enabled
if(NOZ_WEBSOCKET)
    list(APPEND SOURCE_FILES src/websocket.cpp)
endif()

# Add platform-specific files
if(EMSCRIPTEN)
    list(APPEND SOURCE_FILES
        src/platform/web/web_main.cpp
        src/platform/web/web_render.cpp
        src/platform/web/web_input.cpp
        src/platform/web/web_audio.cpp
        src/platform/web/web_time.cpp
        src/platform/gl/gl_render.cpp
    )
    if(NOZ_WEBSOCKET)
        list(APPEND SOURCE_FILES src/platform/web/web_websocket.cpp)
    endif()

    # HTTP backend selection for web
    if(NOZ_HTTP STREQUAL "PLATFORM")
        list(APPEND SOURCE_FILES src/platform/web/web_http.cpp src/http.cpp)
    elseif(NOZ_HTTP STREQUAL "CURL")
        message(FATAL_ERROR "CURL HTTP backend not supported on Emscripten")
    endif()
elseif(WIN32)
    list(APPEND SOURCE_FILES
        src/platform/windows/windows_main.cpp
        src/platform/windows/windows_input.cpp
        src/platform/windows/windows_audio.cpp
        src/platform/windows/windows_time.cpp
    )
    if(NOZ_WEBSOCKET)
        list(APPEND SOURCE_FILES src/platform/windows/windows_websocket.cpp)
    endif()

    # HTTP backend selection for Windows
    if(NOZ_HTTP STREQUAL "CURL")
        list(APPEND SOURCE_FILES src/platform/curl/curl_http.cpp src/http.cpp)
    elseif(NOZ_HTTP STREQUAL "PLATFORM")
        list(APPEND SOURCE_FILES src/platform/windows/windows_http.cpp src/http.cpp)
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
    # HTTP compile definitions for Emscripten
    if(NOZ_HTTP STREQUAL "PLATFORM")
        target_compile_definitions(noz PUBLIC NOZ_HTTP NOZ_HTTP_PLATFORM)
    endif()
    message(STATUS "Building for Web/Emscripten with WebGL and pthreads")
elseif(WIN32)
    target_include_directories(noz PUBLIC
        ${CMAKE_SOURCE_DIR}/res/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan
    )
    target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

    # Add renderer-specific compile definitions
    if(NOZ_RENDERER STREQUAL "GLES")
        target_compile_definitions(noz PUBLIC NOZ_PLATFORM_GLES)
    elseif(NOZ_RENDERER STREQUAL "GL")
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
            -sINITIAL_MEMORY=67108864
            -pthread
            -sPTHREAD_POOL_SIZE=5
    )
    if(NOZ_WEBSOCKET)
        target_link_options(noz PUBLIC -lwebsocket.js)
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
    if(NOZ_HTTP STREQUAL "CURL")
        include(FetchContent)
        set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
        set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
        set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(HTTP_ONLY ON CACHE BOOL "" FORCE)
        set(ENABLE_THREADED_RESOLVER OFF CACHE BOOL "" FORCE)
        set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
        set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
        set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
        set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
        set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
        set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            curl
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG curl-8_11_1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(curl)
        target_link_libraries(noz PUBLIC libcurl_static)
        target_compile_definitions(noz PUBLIC NOZ_HTTP NOZ_HTTP_CURL CURL_STATICLIB)
    elseif(NOZ_HTTP STREQUAL "PLATFORM")
        target_link_libraries(noz PUBLIC winhttp)
        target_compile_definitions(noz PUBLIC NOZ_HTTP NOZ_HTTP_PLATFORM)
    endif()
    # Link winhttp for WebSocket (if not already linked by HTTP PLATFORM)
    if(NOZ_WEBSOCKET AND NOT NOZ_HTTP STREQUAL "PLATFORM")
        target_link_libraries(noz PUBLIC winhttp)
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
