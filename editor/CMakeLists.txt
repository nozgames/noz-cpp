cmake_minimum_required(VERSION 3.10)

project(nozed LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# SDL3_shadercross (local pre-built)

# cgltf
FetchContent_Declare(cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cgltf)

# Create cgltf library target
add_library(cgltf_lib INTERFACE)
target_include_directories(cgltf_lib INTERFACE ${cgltf_SOURCE_DIR})

# Terminal support (Windows uses custom implementation, Unix uses ncurses)
if(NOT WIN32)
    # Use system ncurses on Unix-like systems only
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NCURSES REQUIRED ncurses)
endif()

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# TTF files have been updated to use Stream API and are now included

# Add main nozed executable
add_executable(nozed ${SOURCE_FILES})

# Include directories
target_include_directories(nozed  PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/noz/external
        ${CMAKE_SOURCE_DIR}/libs/noz/include
        ${CMAKE_SOURCE_DIR}/libs/noz/external/sdl_shadercross/include
        ${CMAKE_SOURCE_DIR}/libs/noz/external/enet/include)

# Link libraries
target_link_libraries(nozed PRIVATE
    noz
    cgltf_lib
    ${CMAKE_SOURCE_DIR}/libs/noz/external/sdl_shadercross/SDL3_shadercross-static.lib
    ${CMAKE_SOURCE_DIR}/libs/noz/external/sdl_shadercross/spirv-cross-c-shared.lib
    ${CMAKE_SOURCE_DIR}/libs/noz/external/enet/enet.lib
)

# Link terminal library (only needed for Unix-like systems)
if(NOT WIN32)
    target_link_libraries(nozed PRIVATE ${NCURSES_LIBRARIES})
    target_include_directories(nozed PRIVATE ${NCURSES_INCLUDE_DIRS})
endif()


# Platform-specific libraries
if(WIN32)
    target_link_libraries(nozed PRIVATE
        ws2_32
        winmm
    )
endif()

set_target_properties(nozed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_compile_definitions(nozed PRIVATE _CRT_SECURE_NO_WARNINGS )

# Enable precompiled headers for nozed only (asset_manifest is standalone)
target_precompile_headers(nozed PRIVATE src/editor_pch.h)

# Copy DLLs from external folder to output directory after build (Windows only)
if(WIN32)
    add_custom_command(TARGET nozed POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/libs/noz/external/sdl_shadercross/dxcompiler.dll
        $<TARGET_FILE_DIR:nozed>
        COMMENT "Copying dxcompiler.dll to output directory"
    )
    add_custom_command(TARGET nozed POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/libs/noz/external/sdl_shadercross/spirv-cross-c-shared.dll
        $<TARGET_FILE_DIR:nozed>
        COMMENT "Copying spirv-cross-c-shared.dll to output directory"
    )
endif()
