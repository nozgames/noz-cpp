cmake_minimum_required(VERSION 3.10)

project(nozed LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Terminal support (Windows uses custom implementation, Unix uses ncurses)
if(NOT WIN32)
    # Use system ncurses on Unix-like systems only
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NCURSES REQUIRED ncurses)
endif()

# Explicitly list all source files
set(SOURCE_FILES
    src/asset_manifest.cpp
    src/editor_server.cpp
    src/editor.cpp
    src/file_watcher.cpp
    src/gltf.cpp
    src/importer.cpp
    src/importers/mesh_importer.cpp
    src/importers/font_importer.cpp
    src/importers/shader_importer.cpp
    src/importers/animation_importer.cpp
    src/importers/skeleton_importer.cpp
    src/importers/style_sheet_importer.cpp
    src/importers/vfx_importer.cpp
    src/importers/texture_importer.cpp
    src/importers/sound_importer.cpp
    src/msdf/Contour.cpp
    src/msdf/Edge.cpp
    src/msdf/Shape.cpp
    src/msdf/Math.cpp
    src/msdf/msdf.cpp
    src/msdf/SignedDistance.cpp
    src/rect_packer.cpp
    src/props.cpp
    src/tokenizer.cpp
    src/ttf/TrueTypeFont.cpp
    src/ttf/TrueTypeFontReader.cpp
    src/tui/screen.cpp
    src/tui/text_input.cpp
    src/tui/tstring.cpp
    src/tui/windows_terminal.cpp
    src/views/inspector_object.cpp
    src/views/inspector_view.cpp
    src/views/log_view.cpp
    src/views/properties_view.cpp
    src/views/tree_view.cpp
)

# TTF files have been updated to use Stream API and are now included

# Add main nozed executable
add_executable(nozed ${SOURCE_FILES})

# Include directories
target_include_directories(nozed  PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../external
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/enet/include)

# Link libraries
target_link_libraries(nozed PRIVATE
    noz
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/glslangd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRVd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPVRemapperd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRV-Toolsd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRV-Tools-optd.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/glslang.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRV.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPVRemapper.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRV-Tools.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/../external/glslang/SPIRV-Tools-opt.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/enet/enet.lib
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

target_compile_definitions(nozed PRIVATE _CRT_SECURE_NO_WARNINGS)

# Enable precompiled headers for nozed only (asset_manifest is standalone)
target_precompile_headers(nozed PRIVATE src/editor_pch.h)

