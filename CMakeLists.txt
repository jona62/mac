cmake_minimum_required(VERSION 3.20)
project(Mac)

# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the directories where the header files are located
include_directories(include)

# Add the source files
set(SOURCES
    src/main.cpp
    src/Scanner.cpp  # Scanner implementation is in src/Scanner.cpp
    src/Parser.cpp # Parser implementation is in src/Parser.cpp
    src/Interpreter.cpp # Interpreter implementation is in src/Interpreter.cpp
    src/stb_impl.cpp # stb library implementations
)

# Create the executable
add_executable(mac ${SOURCES})

# Copy assets and stdlib to build directory on every build
add_custom_command(TARGET mac POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/stdlib ${CMAKE_BINARY_DIR}/stdlib
    COMMENT "Copying assets and stdlib to build directory"
)

# Custom target to run the executable
add_custom_target(run
    COMMAND mac
    DEPENDS mac
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running the Mac executable..."
)