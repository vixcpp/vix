# ======================================================
# vix/ - CMake configuration (professional)
# ======================================================
# Modular C++ framework with 'core' submodule
# Handles building core library, umbrella interface,
# and example executables.
# ======================================================

cmake_minimum_required(VERSION 3.20)
project(vix VERSION 0.1 LANGUAGES CXX)

# ======================================================
# Add core submodule as a subdirectory
# ======================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/CMakeLists.txt")
    message(STATUS "Adding 'modules/core' submodule...")
    add_subdirectory(modules/core core_build)
else()
    message(FATAL_ERROR "Missing 'modules/core' submodule. Run:\n"
                        "  git submodule update --init --recursive")
endif()

# ======================================================
# Umbrella 'vix' INTERFACE library
# ======================================================
add_library(vix INTERFACE)

# Include directories exposed to users
target_include_directories(vix INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}              # vix/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
)

# Link the core library to the umbrella interface
target_link_libraries(vix INTERFACE core)

# ======================================================
# Example executable
# ======================================================
add_executable(test_core examples/main.cpp)
target_link_libraries(test_core PRIVATE core)
target_include_directories(test_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
)

# Copy config.json to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ======================================================
# User messages
# ======================================================
message(STATUS "vix configured with core submodule (branch: dev).")
message(STATUS "Example executable 'test_core' configured.")
message(STATUS "Build: cmake --build <build_dir>")
message(STATUS "Run: ./test_core from build directory")
