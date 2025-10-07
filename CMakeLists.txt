# ====================================================================
# Vix.cpp — Umbrella CMake Configuration (Open Source / Maintainers)
# ====================================================================
# Purpose
#   - Single entry point to configure and build the project.
#   - Adds submodules (json, utils, core, cli).
#   - Exposes umbrella INTERFACE target (Vix::vix) so consumers/examples
#     inherit all public headers and transitive dependencies by linking it.
#
# Quick Start
#   $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIX_BUILD_EXAMPLES=ON
#   $ cmake --build build -j
#
# Conventions
#   - C++20 baseline.
#   - Module targets are namespaced with "Vix::" where possible.
#   - Public headers are exposed via target_include_directories(... PUBLIC/INTERFACE).
#   - Avoid global include_directories; prefer target_link_libraries + usage reqs.
#
# Notes for Contributors
#   - If modules/core has no .cpp sources yet, it builds header-only (INTERFACE).
#   - The JSON module may export different target names across environments.
#     We detect them (Vix::json / vix::json / vix_json) to avoid build breaks.
#   - Keep this file readable: future contributors will refer to it often.
#
# License
#   Open source; see LICENSE.
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix VERSION 0.2.0 LANGUAGES CXX)

# ----------------------- Global compiler settings --------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VIX_BUILD_EXAMPLES "Build all umbrella examples in ./examples" ON)

# ----------------------------- Submodules ----------------------------
# Each submodule owns its targets and usage requirements.
# Fail early if a required module is missing.

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/json/CMakeLists.txt")
  message(STATUS "Adding 'modules/json'...")
  add_subdirectory(modules/json json_build)
else()
  message(FATAL_ERROR "Missing 'modules/json'. Run: git submodule update --init --recursive")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/CMakeLists.txt")
  message(STATUS "Adding 'modules/utils'...")
  add_subdirectory(modules/utils utils_build)
else()
  message(FATAL_ERROR "Missing 'modules/utils'. Run: git submodule update --init --recursive")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/CMakeLists.txt")
  message(STATUS "Adding 'modules/core'...")
  add_subdirectory(modules/core core_build)
else()
  message(FATAL_ERROR "Missing 'modules/core'. Run: git submodule update --init --recursive")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cli/CMakeLists.txt")
  message(STATUS "Adding 'modules/cli'...")
  add_subdirectory(modules/cli cli_build)
else()
  message(STATUS "Skipping 'modules/cli' (not present).")
endif()

# ------------------------ Resolve JSON target name -------------------
# Some environments export the JSON target as Vix::json, others as vix::json
# or even a bare vix_json. Detect robustly and reuse a single variable.

set(JSON_TARGET "Vix::json")
if (NOT TARGET ${JSON_TARGET})
  if (TARGET vix::json)
    set(JSON_TARGET vix::json)
  elseif (TARGET vix_json)
    set(JSON_TARGET vix_json)
  else()
    message(FATAL_ERROR
      "JSON module target not found. Expected one of: Vix::json, vix::json, vix_json.\n"
      "Ensure modules/json defines an exported target and alias.")
  endif()
endif()

# ----------------------- Umbrella INTERFACE target -------------------
# Consumers of Vix::vix inherit all public headers and transitive deps
# from core, utils, and json by linking just this one target.

add_library(vix INTERFACE)
add_library(Vix::vix ALIAS vix)

target_link_libraries(vix INTERFACE
  Vix::core
  Vix::utils
  ${JSON_TARGET}
)

# ----------------------------- Examples ------------------------------
# Build any *.cpp in ./examples and link to Vix::vix.
# Header-only helper files (e.g., json_builders_routes.hpp) are ignored here.

if (VIX_BUILD_EXAMPLES)
  file(GLOB VIX_UMBRELLA_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")
  foreach(EXAMPLE_SRC ${VIX_UMBRELLA_EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE Vix::vix)
    # Keep binaries in the top-level build dir for convenience.
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endforeach()
endif()

# --------------------------- Runtime config --------------------------
# Copy a runtime config next to binaries if present.

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/config.json")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# --------------------------- Status summary --------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "Vix umbrella configured")
message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Build umbrella examples: ${VIX_BUILD_EXAMPLES}")
message(STATUS "Modules: json, utils, core (cli optional)")
message(STATUS "Resolved JSON target: ${JSON_TARGET}")
message(STATUS "------------------------------------------------------")
