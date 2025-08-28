cmake_minimum_required(VERSION 3.15)
project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# SDL3
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        main
)

set(SDL3_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)
set(SDL3_SHARED OFF CACHE BOOL "Don't build SDL as shared library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Don't build SDL tests" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Don't build SDL test library" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "Don't install SDL" FORCE)
set(SDL_LIBUDEV OFF CACHE BOOL "Disable udev support" FORCE)
set(SDL_DBUS OFF CACHE BOOL "Disable D-Bus support" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Disable haptic/force feedback support" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Disable sensor support" FORCE)
set(SDL3_VENDORED ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# GLM

# GLM (OpenGL Mathematics)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)

# Box2d
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v2.4.1
)

set(BOX2D_SHARED OFF CACHE BOOL "Don't build Box2D as shared library" FORCE)
set(BOX2D_STATIC ON CACHE BOOL "Build Box2D as static library" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Don't build Box2D testbed" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Don't build Box2D unit tests" FORCE)
set(BOX2D_BUILD_SAMPLES OFF CACHE BOOL "Don't build Box2D samples" FORCE)

# ENet
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.17
)

FetchContent_MakeAvailable(SDL3 box2d glm enet)

add_library(noz STATIC
        include/noz/noz_math.h)

target_precompile_headers(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Remove platform-specific files that don't match the current platform
if(WIN32)
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_posix.cpp")
else()
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_win.cpp")
endif()

target_sources(noz PRIVATE ${SOURCE_FILES})
target_include_directories(noz PRIVATE ${enet_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_include_directories(noz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

# Link dependencies for noz
target_link_libraries(noz
    PUBLIC
        box2d
        enet
        ws2_32
        winmm
        glm::glm
        SDL3::SDL3
)

add_subdirectory(tools/importer)



if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # This makes ninja show shorter status messages
    set(ENV{NINJA_STATUS} "[%f/%t] ")
endif()

