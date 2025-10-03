# ======================================================
# vix/ - CMake configuration
# ======================================================
# This CMakeLists.txt sets up the vix project as a
# modular C++ framework with the 'core' submodule.
# It handles:
#   1. Automatic initialization of the core submodule
#   2. Building the core library
#   3. Creating the umbrella 'vix' interface library
#   4. Building example executables linked to core
# ======================================================

cmake_minimum_required(VERSION 3.20)
project(vix VERSION 0.1 LANGUAGES CXX)

# ======================================================
# Automatic initialization of the 'core' submodule
# ======================================================
execute_process(
    COMMAND git submodule update --init --recursive modules/core
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMODULE_RESULT
)

if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
    message(WARNING "Git submodule 'core' could not be initialized automatically.")
    message(WARNING "Please run: git submodule update --init --recursive")
endif()

# ======================================================
# Add the core submodule as a subdirectory
# ======================================================
add_subdirectory(modules/core core_build)

# ======================================================
# Create the umbrella 'vix' INTERFACE library
# ======================================================
# Exposes include paths and links to core without compiling any sources.
add_library(vix INTERFACE)

# Include directories for umbrella library
target_include_directories(vix INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}              # vix/ for vix/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core # core headers
)

# Link the core library to the umbrella interface
target_link_libraries(vix INTERFACE core)

message(STATUS "vix configured with core submodule.")

# ======================================================
# Example executable: test_core
# ======================================================
add_executable(test_core examples/main.cpp)

# Link the core library to the example executable
target_link_libraries(test_core PRIVATE core)

# Include directories for the executable
target_include_directories(test_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
)

# ======================================================
# Copy the config.json file to the build directory
# ======================================================
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ======================================================
# User information
# ======================================================
message(STATUS "Example executable 'test_core' configured.")
message(STATUS "Build with: cmake --build <build_dir>")
message(STATUS "Run with: ./test_core from the build directory")
