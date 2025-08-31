cmake_minimum_required(VERSION 3.15)
project(noz LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# GLM (OpenGL Mathematics)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)

FetchContent_MakeAvailable(glm)

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
target_include_directories(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/include ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_include_directories(noz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl/include)
target_include_directories(noz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/include)
target_include_directories(noz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_compile_definitions(noz PRIVATE _CRT_SECURE_NO_WARNINGS)

# Link dependencies for noz
target_link_libraries(noz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/box2d/box2d.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/enet/enet.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl/SDL3-static.lib
        ws2_32
        winmm
        setupapi      # For SetupDi* functions (SDL3)
        cfgmgr32      # For CM_* functions (SDL3)
        version       # For GetFileVersionInfo* functions (SDL3)
        imm32         # For Imm* functions (SDL3 IME support)
        dxcompiler    # For DxcCreateInstance (SDL3_shadercross)
        glm::glm
)

add_subdirectory(editor)


if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # This makes ninja show shorter status messages
    set(ENV{NINJA_STATUS} "[%f/%t] ")
endif()

