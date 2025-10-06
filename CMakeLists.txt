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
target_include_directories(vix INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
)
target_link_libraries(vix INTERFACE core)

# ======================================================
# Examples
# ======================================================
file(GLOB EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")

foreach(EXAMPLE_SRC ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE core)
    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
    )
endforeach()

# ======================================================
# Copy config.json to build directory
# ======================================================
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ======================================================
# User messages
# ======================================================
message(STATUS "vix configured with core submodule (branch: dev).")
message(STATUS "Example executables configured for all *.cpp in examples/")
message(STATUS "Build: cmake --build <build_dir>")
message(STATUS "Run: ./<example_name> from build directory")
