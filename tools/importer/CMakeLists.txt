cmake_minimum_required(VERSION 3.10)

project(importer LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# Force static builds globally before declaring dependencies
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)

# SDL3_shadercross (not in parent project)
FetchContent_Declare(SDL3_shadercross
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
    GIT_TAG main
)

set(SDLSHADERCROSS_VENDORED ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_CLI OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_STATIC ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_TOOLS OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_INSTALL OFF CACHE BOOL "" FORCE)

# Force SPIRV-Cross to build statically
set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_TOOLS_SKIP_SYMBOL_EXPORT ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)

# SPIRV-Tools requires C++17
set(CMAKE_CXX_STANDARD_SPIRV_TOOLS 17 CACHE STRING "" FORCE)
set(SPIRV_TOOLS_CXX_STANDARD 17 CACHE STRING "" FORCE)
    
# Keep DXC components needed by SDL3_shadercross
set(DXC_BUILD_EXE OFF)
set(DXC_BUILD_CLI OFF)
set(DXC_BUILD_TOOLS OFF)

# Disable HLSL tests to speed up build
set(HLSL_BUILD_TESTS OFF)
set(HLSL_ENABLE_TESTS OFF)
set(CLANG_BUILD_TESTS OFF)
set(LLVM_BUILD_TESTS OFF)
set(LLVM_INCLUDE_TESTS OFF)
# Disable DXC installation to avoid distribution component error
set(LLVM_DISTRIBUTION_COMPONENTS "" CACHE STRING "" FORCE)
set(LLVM_INSTALL_TOOLCHAIN_ONLY ON CACHE BOOL "" FORCE)
set(LLVM_INSTALL_UTILS OFF CACHE BOOL "" FORCE)

# Save current C++ standard
set(SAVED_CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})
set(SAVED_CMAKE_CXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})

# Set C++17 for SPIRV-Tools
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_MakeAvailable(SDL3_shadercross)

# Restore original C++ standard for rest of the project
set(CMAKE_CXX_STANDARD ${SAVED_CMAKE_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ${SAVED_CMAKE_CXX_STANDARD_REQUIRED})

# cgltf
FetchContent_Declare(cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cgltf)

# Create cgltf library target
add_library(cgltf_lib INTERFACE)
target_include_directories(cgltf_lib INTERFACE ${cgltf_SOURCE_DIR})

file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp")

# TTF files have been updated to use Stream API and are now included

# Add main importer executable
add_executable(importer ${SOURCE_FILES})

# Include directories
target_include_directories(importer PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/noz/external
        ${CMAKE_SOURCE_DIR}/libs/noz/include
        ${CMAKE_SOURCE_DIR}/libs/noz/tools/common
        ${sdl3_shadercross_SOURCE_DIR}/external/SPIRV-Cross
        ${enet_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(importer PRIVATE
    noz
    cgltf_lib
    SDL3_shadercross::SDL3_shadercross
    enet
)


# Platform-specific libraries
if(WIN32)
    target_link_libraries(importer PRIVATE
        ws2_32
        winmm
    )
endif()

set_target_properties(importer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_compile_definitions(importer PRIVATE _CRT_SECURE_NO_WARNINGS )

# Enable precompiled headers for importer only (asset_manifest is standalone)
target_precompile_headers(importer PRIVATE src/importer_pch.h)

# Copy dxcompiler.dll to output directory after build (Windows only)
if(WIN32)
    add_custom_command(TARGET importer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dxcompiler>
        $<TARGET_FILE_DIR:importer>
        COMMENT "Copying dxcompiler.dll to output directory"
    )
endif()
