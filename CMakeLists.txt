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

# --- SANITIZERS (GLOBAL & ROBUSTE) ------------------------------------
option(VIX_ENABLE_SANITIZERS "Enable Address/Undefined Sanitizers" OFF)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if (VIX_ENABLE_SANITIZERS)
  if (MSVC)
    message(FATAL_ERROR "Sanitizers not supported on MSVC. Use Clang or GCC.")
  endif()

  # Flags communs
  set(_SAN_COMMON -O1 -g -fno-omit-frame-pointer)
  set(_SAN_TYPES  -fsanitize=address,undefined)

  add_compile_options(${_SAN_COMMON} ${_SAN_TYPES})
  add_link_options(${_SAN_TYPES})

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC sanitizers (assurez-vous que libasan/libubsan sont installées).")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang sanitizers (compiler-rt).")
  endif()

  function(vix_apply_sanitizers tgt)
    if (TARGET ${tgt})
      target_compile_options(${tgt} PRIVATE ${_SAN_COMMON} ${_SAN_TYPES})
      target_link_options(${tgt} PRIVATE ${_SAN_TYPES})
    endif()
  endfunction()
endif()

include(CMakeDependentOption)
include(FetchContent)

# ----------------------------------------------------
# Always export compile_commands.json (for VS Code)
# ----------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy compile_commands.json to the project root on every build
add_custom_target(copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Copy compile_commands.json to project root for VS Code"
)

# ----------------------- Global compiler settings --------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Nice warnings pack (applied via INTERFACE to dependents)
option(VIX_ENABLE_WARNINGS "Enable extra warnings" ON)
if (VIX_ENABLE_WARNINGS)
  add_library(vix_warnings INTERFACE)
  if (MSVC)
    target_compile_options(vix_warnings INTERFACE /W4 /permissive-)
  else()
    target_compile_options(vix_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
  endif()
endif()

# Optional LTO for Release
option(VIX_ENABLE_LTO "Enable link-time optimization (Release only)" OFF)
if (VIX_ENABLE_LTO AND CMAKE_BUILD_TYPE MATCHES "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_err)
  if (ipo_ok)
    message(STATUS "IPO/LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_err}")
  endif()
endif()

# Static analysis hooks (opt-in)
option(VIX_ENABLE_CLANG_TIDY "Enable clang-tidy if available" OFF)
if (VIX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if (CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "clang-tidy requested but not found")
  endif()
endif()

option(VIX_ENABLE_CPPCHECK "Enable cppcheck if available" OFF)
if (VIX_ENABLE_CPPCHECK)
  find_program(CPPCHECK_EXE NAMES cppcheck)
  if (CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK
      ${CPPCHECK_EXE}
      --enable=warning,style,performance,portability
      --std=c++20 --inline-suppr
      --suppress=unusedFunction) # tune as needed
    message(STATUS "cppcheck enabled: ${CPPCHECK_EXE}")
  else()
    message(WARNING "cppcheck requested but not found")
  endif()
endif()

# Coverage (GCC/Clang, Debug)
option(VIX_ENABLE_COVERAGE "Enable coverage flags (Debug only)" OFF)
if (VIX_ENABLE_COVERAGE AND CMAKE_BUILD_TYPE MATCHES "Debug")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_library(vix_coverage INTERFACE)
    target_compile_options(vix_coverage INTERFACE --coverage -O0 -g)
    target_link_options(vix_coverage INTERFACE --coverage)
    message(STATUS "Coverage flags enabled")
  else()
    message(WARNING "Coverage only supported on GCC/Clang")
  endif()
endif()

option(VIX_BUILD_EXAMPLES "Build all umbrella examples in ./examples" ON)
option(VIX_BUILD_TESTS "Build unit tests (GoogleTest)" ON)

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

if (VIX_ENABLE_SANITIZERS)
  vix_apply_sanitizers(vix_core)
  vix_apply_sanitizers(vix_utils)
  vix_apply_sanitizers(vix_cli)        

  vix_apply_sanitizers(vix_tests)

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

# Propagate warning pack / coverage if enabled
if (TARGET vix_warnings)
  target_link_libraries(vix INTERFACE vix_warnings)
endif()
if (TARGET vix_coverage)
  target_link_libraries(vix INTERFACE vix_coverage)
endif()

# ---------------------- Sanitizers (optional) ------------------------
# Enable correct link of ASan/UBSan when desired.
option(VIX_ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)

if (VIX_ENABLE_SANITIZERS)
  message(STATUS "VIX: enabling sanitizers (ASan + UBSan)")
  # Recommended flags for better stack traces with ASan
  target_compile_options(vix INTERFACE -O1 -g -fno-omit-frame-pointer)
  # Apply to both compile and link stages (propagates to dependents)
  target_compile_options(vix INTERFACE -fsanitize=address,undefined)
  target_link_options(vix INTERFACE -fsanitize=address,undefined)
endif()

# ----------------------------- Examples ------------------------------
# Build any *.cpp in ./examples and link to Vix::vix.
# Header-only helper files (e.g., json_builders_routes.hpp) are ignored here.

# ----------------------------- Examples ------------------------------
# Build any *.cpp in ./examples and link to Vix::vix.
# - Auto-regenerate on file add/remove (CONFIGURE_DEPENDS)
# - Ignore helpers like *_common.cpp or *_disabled.cpp
# - Avoid target-name collisions (prefix ex_ if TARGET exists)
# - Keep binaries in the top-level build dir for convenience.

if (VIX_BUILD_EXAMPLES)
  file(GLOB VIX_UMBRELLA_EXAMPLES
       CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")

  if (NOT VIX_UMBRELLA_EXAMPLES)
    message(STATUS "No example sources found under examples/. Skipping.")
  else()
    foreach(EXAMPLE_SRC IN LISTS VIX_UMBRELLA_EXAMPLES)
      get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)

      # Skip helper/disabled files
      if (_raw_name MATCHES "_common$" OR _raw_name MATCHES "_disabled$")
        message(STATUS "Skipping helper example: ${_raw_name}")
        continue()
      endif()

      # Avoid collisions with existing targets
      set(EXAMPLE_NAME "${_raw_name}")
      if (TARGET "${EXAMPLE_NAME}")
        set(EXAMPLE_NAME "ex_${_raw_name}")
        message(WARNING "Target name '${_raw_name}' already exists. Using '${EXAMPLE_NAME}'.")
      endif()

      add_executable("${EXAMPLE_NAME}" "${EXAMPLE_SRC}")
      target_link_libraries("${EXAMPLE_NAME}" PRIVATE Vix::vix)

      # Optional: appliquer warnings/sanitizers/coverage aux exemples
      if (TARGET vix_warnings)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_warnings)
      endif()
      if (VIX_ENABLE_SANITIZERS)
        target_compile_options("${EXAMPLE_NAME}" PRIVATE -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
        target_link_options("${EXAMPLE_NAME}" PRIVATE -fsanitize=address,undefined)
      endif()
      if (TARGET vix_coverage)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_coverage)
      endif()

      # Place binaries in the top-level build dir
      set_target_properties("${EXAMPLE_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
      )

      message(STATUS "Example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
    endforeach()
  endif()
endif()


# ------------------------------ Tests --------------------------------
if (VIX_BUILD_TESTS)
  include(CTest)          # enables 'make test' / 'ctest'
  enable_testing()

  # Try system GTest first, else FetchContent (no internet in some CI? adjust if needed)
  find_package(GTest QUIET)
  if (NOT GTest_FOUND)
    message(STATUS "GTest not found, fetching googletest...")
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.14.0
    )
    # Windows DLL runtime settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  # Example test target (auto-discovers *_test.cpp under tests/)
  file(GLOB_RECURSE VIX_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp")
  if (VIX_TEST_SOURCES)
    add_executable(vix_tests ${VIX_TEST_SOURCES})
    target_link_libraries(vix_tests PRIVATE
      Vix::vix
      GTest::gtest
      GTest::gtest_main
    )
    if (TARGET vix_warnings)
      target_link_libraries(vix_tests PRIVATE vix_warnings)
    endif()
    if (TARGET vix_coverage)
      target_link_libraries(vix_tests PRIVATE vix_coverage)
    endif()

    include(GoogleTest)
    gtest_discover_tests(vix_tests
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DISCOVERY_MODE PRE_TEST
    )
  else()
    message(STATUS "No *_test.cpp found under tests/ — skipping test binary generation.")
  endif()
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
message(STATUS "Build tests (GTest): ${VIX_BUILD_TESTS}")
message(STATUS "Modules: json, utils, core (cli optional)")
message(STATUS "Resolved JSON target: ${JSON_TARGET}")
message(STATUS "Sanitizers: ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Coverage: ${VIX_ENABLE_COVERAGE}")
message(STATUS "Clang-Tidy: ${VIX_ENABLE_CLANG_TIDY}")
message(STATUS "Cppcheck: ${VIX_ENABLE_CPPCHECK}")
message(STATUS "LTO: ${VIX_ENABLE_LTO}")
message(STATUS "------------------------------------------------------")
