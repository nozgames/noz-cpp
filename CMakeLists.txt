cmake_minimum_required(VERSION 3.15)

project(noz LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SDL3
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.16
)
set(SDL3_SHARED OFF CACHE BOOL "" FORCE)
set(SDL3_STATIC ON CACHE BOOL "" FORCE)
set(SDL3_VENDORED ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Box2D physics library
FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v2.4.1
)
set(BOX2D_SHARED OFF)
set(BOX2D_STATIC ON)
set(BOX2D_BUILD_TESTBED OFF)
set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_SAMPLES OFF)
FetchContent_MakeAvailable(box2d)

# GLM (OpenGL Mathematics)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# ENet
FetchContent_Declare(enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.17
)
set(ENET_BUILD_SHARED OFF)
set(ENET_BUILD_STATIC ON)
FetchContent_MakeAvailable(enet)

# Suppress warnings for ENet after it's available
if(TARGET enet)
    if(MSVC)
        target_compile_options(enet PRIVATE /w)
    else()
        target_compile_options(enet PRIVATE -w)
    endif()
endif()

# SDL3_image
FetchContent_Declare(SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
)
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_STATIC ON CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)  # Avoid NASM dependency
FetchContent_MakeAvailable(SDL3_image)

add_library(noz STATIC)

# Enable precompiled headers
target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/noz_pch.h)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

target_sources(noz PRIVATE ${SOURCE_FILES})

target_include_directories(noz PRIVATE ${enet_SOURCE_DIR}/include)
target_include_directories(noz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(noz
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

# Link dependencies for noz
target_link_libraries(noz
    PUBLIC
        box2d
        enet
        ws2_32
        winmm
        SDL3_image::SDL3_image
        SDL3::SDL3
        glm::glm
)
