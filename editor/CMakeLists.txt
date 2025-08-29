cmake_minimum_required(VERSION 3.10)

project(nozed LANGUAGES C CXX)
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

# Find ncurses
if(WIN32)
    # Use PDCurses on Windows
    FetchContent_Declare(pdcurses
        GIT_REPOSITORY https://github.com/wmcbrine/PDCurses.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(pdcurses)
    
    # Create pdcurses library
    add_library(pdcurses_lib STATIC
        ${pdcurses_SOURCE_DIR}/wincon/pdcclip.c
        ${pdcurses_SOURCE_DIR}/wincon/pdcdisp.c
        ${pdcurses_SOURCE_DIR}/wincon/pdcgetsc.c
        ${pdcurses_SOURCE_DIR}/wincon/pdckbd.c
        ${pdcurses_SOURCE_DIR}/wincon/pdcscrn.c
        ${pdcurses_SOURCE_DIR}/wincon/pdcsetsc.c
        ${pdcurses_SOURCE_DIR}/wincon/pdcutil.c
        ${pdcurses_SOURCE_DIR}/pdcurses/addch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/addchstr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/addstr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/attr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/beep.c
        ${pdcurses_SOURCE_DIR}/pdcurses/bkgd.c
        ${pdcurses_SOURCE_DIR}/pdcurses/border.c
        ${pdcurses_SOURCE_DIR}/pdcurses/clear.c
        ${pdcurses_SOURCE_DIR}/pdcurses/color.c
        ${pdcurses_SOURCE_DIR}/pdcurses/delch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/deleteln.c
        ${pdcurses_SOURCE_DIR}/pdcurses/getch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/getstr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/getyx.c
        ${pdcurses_SOURCE_DIR}/pdcurses/inch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/inchstr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/initscr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/inopts.c
        ${pdcurses_SOURCE_DIR}/pdcurses/insch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/insstr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/instr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/kernel.c
        ${pdcurses_SOURCE_DIR}/pdcurses/keyname.c
        ${pdcurses_SOURCE_DIR}/pdcurses/mouse.c
        ${pdcurses_SOURCE_DIR}/pdcurses/move.c
        ${pdcurses_SOURCE_DIR}/pdcurses/outopts.c
        ${pdcurses_SOURCE_DIR}/pdcurses/overlay.c
        ${pdcurses_SOURCE_DIR}/pdcurses/pad.c
        ${pdcurses_SOURCE_DIR}/pdcurses/panel.c
        ${pdcurses_SOURCE_DIR}/pdcurses/printw.c
        ${pdcurses_SOURCE_DIR}/pdcurses/refresh.c
        ${pdcurses_SOURCE_DIR}/pdcurses/scanw.c
        ${pdcurses_SOURCE_DIR}/pdcurses/scr_dump.c
        ${pdcurses_SOURCE_DIR}/pdcurses/scroll.c
        ${pdcurses_SOURCE_DIR}/pdcurses/slk.c
        ${pdcurses_SOURCE_DIR}/pdcurses/termattr.c
        ${pdcurses_SOURCE_DIR}/pdcurses/touch.c
        ${pdcurses_SOURCE_DIR}/pdcurses/util.c
        ${pdcurses_SOURCE_DIR}/pdcurses/window.c
    )
    target_include_directories(pdcurses_lib PUBLIC ${pdcurses_SOURCE_DIR})
    target_compile_definitions(pdcurses_lib PUBLIC PDC_WIDE)
else()
    # Use system ncurses on Unix-like systems
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
        ${sdl3_shadercross_SOURCE_DIR}/external/SPIRV-Cross
        ${enet_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(nozed PRIVATE
    noz
    cgltf_lib
    SDL3_shadercross::SDL3_shadercross
    enet
)

# Link curses library
if(WIN32)
    target_link_libraries(nozed PRIVATE pdcurses_lib)
else()
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

# Copy dxcompiler.dll to output directory after build (Windows only)
if(WIN32)
    add_custom_command(TARGET nozed POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dxcompiler>
        $<TARGET_FILE_DIR:nozed>
        COMMENT "Copying dxcompiler.dll to output directory"
    )
endif()
