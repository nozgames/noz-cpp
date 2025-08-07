cmake_minimum_required(VERSION 3.10)

project(icongen)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(icongen
    src/icongen.cpp
    src/border.cpp
    src/border.h
)

# Include directories
target_include_directories(icongen PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/libs/noz/include
)

# Link libraries
target_link_libraries(icongen PRIVATE
    noz
    cxxopts::cxxopts
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(icongen PRIVATE
        ws2_32
        winmm
    )
endif()

# Set output directory
set_target_properties(icongen PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Enable precompiled headers
target_precompile_headers(icongen PRIVATE
    src/icongen_pch.h
)
