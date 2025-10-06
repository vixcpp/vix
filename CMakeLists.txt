# ======================================================
# vix/ - CMake configuration (professional)
# ======================================================
# Modular C++ framework with 'core', 'cli' and 'utils' submodules
# ======================================================

cmake_minimum_required(VERSION 3.20)
project(vix VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ======================================================
# Utils submodule
# ======================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/CMakeLists.txt")
    message(STATUS "Adding 'modules/utils' submodule...")
    add_subdirectory(modules/utils utils_build)
else()
    message(FATAL_ERROR "Missing 'modules/utils' submodule. Run:\n"
                        "  git submodule update --init --recursive")
endif()

# ======================================================
# Core submodule
# ======================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/CMakeLists.txt")
    message(STATUS "Adding 'modules/core' submodule...")
    add_subdirectory(modules/core core_build)
else()
    message(FATAL_ERROR "Missing 'modules/core' submodule. Run:\n"
                        "  git submodule update --init --recursive")
endif()

# ======================================================
# CLI submodule
# ======================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cli/CMakeLists.txt")
    message(STATUS "Adding 'modules/cli' submodule...")
    add_subdirectory(modules/cli cli_build)
else()
    message(WARNING "Missing 'modules/cli' submodule. CLI won't be built.")
endif()

# ======================================================
# Umbrella 'vix' INTERFACE library
# ======================================================
add_library(vix INTERFACE)
target_include_directories(vix INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}               
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core 
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/include 
)
target_link_libraries(vix INTERFACE core utils)

# ======================================================
# Examples
# ======================================================
file(GLOB EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")

foreach(EXAMPLE_SRC ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})

    # Inclure core et utils
    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/core
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/include
    )

    target_link_libraries(${EXAMPLE_NAME} PRIVATE core utils)
endforeach()

# ======================================================
# Copy config.json to build directory
# ======================================================
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ======================================================
# User messages
# ======================================================
message(STATUS "vix configured with core, utils (and CLI if present).")
message(STATUS "Example executables configured for all *.cpp in examples/")
message(STATUS "Build: cmake --build <build_dir>")
message(STATUS "Run: ./<example_name> or ./vix (CLI)")
