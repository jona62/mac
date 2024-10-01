cmake_minimum_required(VERSION 3.20)
project(Mac)

# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the directories where the header files are located
include_directories(include)

# Add the source files
set(SOURCES
    main.cpp
    src/Scanner.cpp  # Scanner implementation is in src/Scanner.cpp
    src/Parser.cpp # Parser implementation is in src/Parser.cpp
)

# Create the executable
add_executable(mac ${SOURCES})

# Custom target to run the executable
add_custom_target(run
    COMMAND mac
    DEPENDS mac
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running the Mac executable..."
)