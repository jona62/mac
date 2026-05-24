cmake_minimum_required(VERSION 3.20)
project(Mac)

# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Read version from VERSION file
file(READ "${CMAKE_SOURCE_DIR}/VERSION" MAC_VERSION)
string(STRIP "${MAC_VERSION}" MAC_VERSION)
add_compile_definitions(MAC_VERSION="${MAC_VERSION}")

# Include the directories where the header files are located
include_directories(include)

# Add the source files
set(SOURCES
    src/main.cpp
    src/Scanner.cpp
    src/Parser.cpp
    src/Interpreter.cpp
    src/stb_impl.cpp
)

if(NOT WIN32)
    list(APPEND SOURCES src/linenoise.c)
endif()

# Create the executable
add_executable(mac ${SOURCES})

# MSVC-specific flags
if(MSVC)
    target_compile_options(mac PRIVATE /utf-8)
    target_compile_definitions(mac PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Copy assets and stdlib to build directory on every build
add_custom_target(copy_runtime_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/stdlib ${CMAKE_BINARY_DIR}/stdlib
    COMMENT "Copying assets and stdlib to build directory"
)

add_dependencies(mac copy_runtime_assets)

# Custom target to run the executable
add_custom_target(run
    COMMAND mac
    DEPENDS mac
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running the Mac executable..."
)
