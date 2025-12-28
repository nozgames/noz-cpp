cmake_minimum_required(VERSION 3.15)

project(nozed)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(NOZ_LUA ON CACHE BOOL "" FORCE)
set(NOZ_HTTP ON CACHE BOOL "" FORCE)
set(NOZ_WEBSOCKET ON CACHE BOOL "" FORCE)
option(NOZ_EDITOR_LIB "Build nozed as a static library instead of executable" OFF)

add_subdirectory(../ noz)
target_compile_definitions(noz PUBLIC NOZ_EDITOR)

file(GLOB SRC_FILES
    src/*.cpp
    src/*.h
    src/**/*.cpp
    src/**/*.h
)
set(SOURCE_FILES ${SRC_FILES})

if(NOZ_EDITOR_LIB)
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*nozed_assets\\.cpp$")
    add_library(nozed STATIC ${SOURCE_FILES})
    target_compile_definitions(nozed PUBLIC NOZ_EDITOR_LIB)
else()
    add_executable(nozed ${SOURCE_FILES} res/windows/nozed.rc)
endif()

target_compile_definitions(nozed PRIVATE _CRT_SECURE_NO_WARNINGS NOZ_LUA)
target_link_libraries(nozed PRIVATE
    noz
    debug ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/glslangd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRVd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPVRemapperd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRV-Toolsd.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRV-Tools-optd.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/glslang.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRV.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPVRemapper.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRV-Tools.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/SPIRV-Tools-opt.lib
)

if(MSVC)
    target_compile_options(nozed PRIVATE /W4 /WX)
endif()

if(NOT NOZ_EDITOR_LIB)
    set_target_properties(nozed PROPERTIES WIN32_EXECUTABLE TRUE)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT nozed)
endif()
target_precompile_headers(nozed PUBLIC src/pch.h)
target_include_directories(nozed PRIVATE
    src
    external
    res/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/../external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glslang/include
)
