cmake_minimum_required(VERSION 3.20)

# ====================================================================
# Vix.cpp — Umbrella CMake Configuration (Open Source / Maintainers)
# ====================================================================
# Purpose
#   - Single entry point to configure and build the project.
#   - Adds submodules (json, utils, core, orm, cli).
#   - Exposes an umbrella INTERFACE target (vix::vix) so consumers/examples
#     inherit public headers and transitive dependencies by linking it.
#
# Quick Start
#   $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIX_BUILD_EXAMPLES=ON
#   $ cmake --build build -j
#
# Conventions
#   - C++20 baseline.
#   - Module targets are namespaced with "vix::" where possible.
#   - Prefer target_link_libraries + usage requirements over global includes.
#
# Notes for Contributors
#   - If modules/core has no .cpp sources yet, it builds header-only (INTERFACE).
#   - The JSON module may export different target names across environments:
#     vix::json / vix::json / vix_json — we resolve them dynamically.
# ====================================================================
project(vix VERSION 1.10.4 LANGUAGES CXX)

# Make find_package honor *_ROOT hints (e.g. MYSQLCPPCONN_ROOT)
if (POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
# RPATH to easily run after installation
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
if (APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()
include(CMakeDependentOption)
include(FetchContent)

# ----------------------------------------------------
# Build settings & developer options
# ----------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Export compile_commands.json (for VS Code, clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Copy compile_commands.json to project root for tooling"
)

# Warnings bundle (opt-in)
option(VIX_ENABLE_WARNINGS "Enable extra warnings" ON)
if (VIX_ENABLE_WARNINGS)
  add_library(vix_warnings INTERFACE)
  if (MSVC)
    target_compile_options(vix_warnings INTERFACE /W4 /permissive-)
  else()
    target_compile_options(vix_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    # 👇 Silence des faux positifs GCC 13 (fmt/spdlog inlines)
    target_compile_options(vix_warnings INTERFACE
      $<$<CXX_COMPILER_ID:GNU>:-Wno-array-bounds>
    )
  endif()
endif()

# LTO (Release)
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

option(VIX_MSVC_STATIC_RUNTIME "Link MSVC runtime statically (/MT)" OFF)
if (MSVC AND VIX_MSVC_STATIC_RUNTIME)
  foreach(flag_var
    CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_DEBUG)
    if (DEFINED ${flag_var})
      string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
  endforeach()
endif()

# Static analysis (opt-in)
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
      --suppress=unusedFunction)
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

# Sanitizers (opt-in)
option(VIX_ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
if (VIX_ENABLE_SANITIZERS)
  if (MSVC)
    message(FATAL_ERROR "Sanitizers are not supported on MSVC. Use Clang or GCC.")
  endif()
  set(_SAN_COMMON -O1 -g -fno-omit-frame-pointer)
  set(_SAN_TYPES  -fsanitize=address,undefined)
  add_compile_options(${_SAN_COMMON} ${_SAN_TYPES})
  add_link_options(${_SAN_TYPES})
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC sanitizers (ensure libasan/libubsan are installed).")
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

option(VIX_BUILD_EXAMPLES "Build all umbrella examples in ./examples" ON)
option(VIX_BUILD_TESTS "Build unit tests (GoogleTest)" ON)

# ----------------------------------------------------
# Umbrella INTERFACE target — create it EARLY
# ----------------------------------------------------
add_library(vix INTERFACE)
add_library(vix::vix ALIAS vix)

# Public headers visibility (build/install)
target_include_directories(vix INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/json/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Attach warnings/coverage to umbrella (propagates to consumers)
if (TARGET vix_warnings)
  target_link_libraries(vix INTERFACE vix_warnings)
endif()
if (TARGET vix_coverage)
  target_link_libraries(vix INTERFACE vix_coverage)
endif()

# ----------------------------------------------------
# Submodules
# ----------------------------------------------------
option(VIX_FORCE_FETCH_JSON "Fetch a JSON backend if 'modules/json' submodule is missing" ON)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/json/CMakeLists.txt")
  message(STATUS "Adding 'modules/json'...")
  add_subdirectory(modules/json json_build)
  set(_VIX_JSON_BACKEND "submodule")
elseif (VIX_FORCE_FETCH_JSON)
  message(WARNING "modules/json submodule not found — fetching nlohmann/json as fallback")
  include(FetchContent)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)

  add_library(vix_json INTERFACE)
# après avoir défini vix_json
if (NOT TARGET vix::json)
  add_library(vix::json ALIAS vix_json)
else()
  message(STATUS "[json] alias vix::json already exists — skipping alias creation")
endif()

  target_link_libraries(vix_json INTERFACE nlohmann_json::nlohmann_json)

  set(_VIX_JSON_BACKEND "nlohmann_json")
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

# CLI (tool)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cli/CMakeLists.txt")
  message(STATUS "Adding 'modules/cli'...")
  add_subdirectory(modules/cli cli_build)
else()
  message(WARNING "Skipping 'modules/cli' (not present).")
endif()

# # ----------------------------------------------------
# # ORM (optionnel)
# # ----------------------------------------------------
# option(VIX_ENABLE_ORM "Build Vix ORM module" OFF)

# # Conserve les options internes sans FORCE
# set(VIX_ORM_USE_MYSQL      ON  CACHE BOOL "Enable MySQL backend in vix_orm")
# set(VIX_ORM_BUILD_EXAMPLES OFF CACHE BOOL "Build vix_orm examples")
# set(VIX_ORM_BUILD_TESTS    OFF CACHE BOOL "Build vix_orm tests")

# # Variable exportée vers VixConfig.cmake.in
# set(VIX_HAS_ORM OFF)

# if (VIX_ENABLE_ORM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/orm/CMakeLists.txt")
#   message(STATUS "Adding 'modules/orm'...")
#   add_subdirectory(modules/orm orm_build)
#   if (TARGET vix::orm)
#     target_link_libraries(vix INTERFACE vix::orm)
#     set(VIX_HAS_ORM ON)
#   endif()
# endif()

# ----------------------------------------------------
# Resolve JSON target name for downstreams
# ----------------------------------------------------
set(JSON_TARGET "vix::json")
if (NOT TARGET ${JSON_TARGET})
  if (TARGET vix::json)
    set(JSON_TARGET vix::json)
  elseif (TARGET vix_json)
    set(JSON_TARGET vix_json)
  else()
    message(FATAL_ERROR "JSON module target not found ...")
  endif()
endif()

# Link umbrella to core/utils/json
target_link_libraries(vix INTERFACE
  vix::core
  vix::utils
  ${JSON_TARGET}
)

# Propagate sanitizers to umbrella (and thus to consumers)
if (VIX_ENABLE_SANITIZERS)
  message(STATUS "VIX: enabling sanitizers (ASan + UBSan) on umbrella target")
  target_compile_options(vix INTERFACE -O1 -g -fno-omit-frame-pointer)
  target_compile_options(vix INTERFACE -fsanitize=address,undefined)
  target_link_options(vix INTERFACE -fsanitize=address,undefined)
endif()

# ----------------------------------------------------
# Examples
# ----------------------------------------------------
# Security: Ensure VIX_HAS_ORM is defined (core-only => OFF)
if (NOT DEFINED VIX_HAS_ORM)
  set(VIX_HAS_ORM OFF)
endif()

if (VIX_BUILD_EXAMPLES)
  file(GLOB VIX_UMBRELLA_EXAMPLES
       CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")

  # Examples that require ORM
  set(_ORM_EXAMPLES
    demo_with_orm
    error_handling
    batch_insert_tx
    users_crud
    repository_crud_full
    migrate_init
    tx_unit_of_work
    querybuilder_update
  )

# If the ORM is not packaged, filter them from the list
  if (NOT VIX_HAS_ORM)
    foreach(_ex IN LISTS _ORM_EXAMPLES)
      # POSIX paths
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*/${_ex}\\.cpp$")
      # Windows paths (just in case)
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*\\\\${_ex}\\.cpp$")
    endforeach()
    message(STATUS "Examples: ORM is OFF → filtered ORM-dependent samples: ${_ORM_EXAMPLES}")
  endif()

  if (NOT VIX_UMBRELLA_EXAMPLES)
    message(STATUS "No example sources found under examples/. Skipping.")
  else()
    foreach(EXAMPLE_SRC IN LISTS VIX_UMBRELLA_EXAMPLES)
      get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)
      if (_raw_name MATCHES "_common$" OR _raw_name MATCHES "_disabled$" OR _raw_name MATCHES "_internal$")
        message(STATUS "Skipping helper/internal example: ${_raw_name}")
        continue()
      endif()

      set(EXAMPLE_NAME "${_raw_name}")
      if (TARGET "${EXAMPLE_NAME}")
        set(EXAMPLE_NAME "ex_${_raw_name}")
        message(WARNING "Target name '${_raw_name}' already exists. Using '${EXAMPLE_NAME}'.")
      endif()

      add_executable("${EXAMPLE_NAME}" "${EXAMPLE_SRC}")
      target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix::vix)

      if (TARGET vix_warnings)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_warnings)
      endif()
      if (VIX_ENABLE_SANITIZERS)
        target_compile_options("${EXAMPLE_NAME}" PRIVATE -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
        target_link_options("${EXAMPLE_NAME}" PRIVATE                       -fsanitize=address,undefined)
      endif()
      if (TARGET vix_coverage)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_coverage)
      endif()

      set_target_properties("${EXAMPLE_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
      )

      message(STATUS "Example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
    endforeach()
  endif()
endif()

# ----------------------------------------------------
# Tests
# ----------------------------------------------------
if (VIX_BUILD_TESTS)
  include(CTest)
  enable_testing()

  find_package(GTest QUIET)
  if (NOT GTest_FOUND)
    message(STATUS "GTest not found, fetching googletest...")
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  file(GLOB_RECURSE VIX_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp")
  if (VIX_TEST_SOURCES)
    add_executable(vix_tests ${VIX_TEST_SOURCES})
    target_link_libraries(vix_tests PRIVATE
      vix::vix
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

# ----------------------------------------------------
# Runtime config (copy optional config.json next to binaries)
# ----------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/config.json")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# ----------------------------------------------------
# Install/export (umbrella package)
# ----------------------------------------------------
option(VIX_ENABLE_INSTALL "Enable install/export rules for packaging" ON)

if (VIX_ENABLE_INSTALL)
  # 1) Headers publics (interfaces de build/install)
  install(DIRECTORY modules/core/include/  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  install(DIRECTORY modules/utils/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  install(DIRECTORY modules/json/include/  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  # Si l’ORM expose des headers publics :
  # install(DIRECTORY modules/orm/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  #         FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

  # 2) Export set — on exporte la cible INTERFACE 'vix'
  install(TARGETS vix
          EXPORT VixTargets)

  # Exporter aussi les INTERFACE auxiliaires si elles existent
  if (TARGET vix_warnings)
    install(TARGETS vix_warnings
            EXPORT VixTargets)
  endif()
  if (TARGET vix_coverage)
    install(TARGETS vix_coverage
            EXPORT VixTargets)
  endif()

  # 3) Config files pour find_package(Vix)
  set(VIX_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Vix")

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Fichier de config (fourni par cmake/VixConfig.cmake.in)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VixConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
    INSTALL_DESTINATION "${VIX_INSTALL_CMAKEDIR}"
  )

  # 4) Export réel des cibles
  install(EXPORT VixTargets
          NAMESPACE vix::
          DESTINATION "${VIX_INSTALL_CMAKEDIR}")

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
    DESTINATION "${VIX_INSTALL_CMAKEDIR}"
  )

    # 5) Installer le binaire CLI s’il existe — NE PAS l’exporter
  #    - Si la cible s'appelle 'vix_cli', on l'installe sous le nom 'vix'
  #    - Si la cible s'appelle déjà 'vix', on l'installe tel quel
  if (TARGET vix_cli)
    install(TARGETS vix_cli
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            RENAME vix)
    message(STATUS "CLI install: installing target 'vix_cli' as 'vix'")
  elseif (TARGET vix)
    install(TARGETS vix
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    message(STATUS "CLI install: installing target 'vix'")
  else()
    message(WARNING "CLI install: no target named 'vix_cli' or 'vix' — CLI will not be installed")
  endif()

endif()

# ----------------------------------------------------
# Status summary
# ----------------------------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "Vix umbrella configured")
message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Build umbrella examples: ${VIX_BUILD_EXAMPLES}")
message(STATUS "Build tests (GTest): ${VIX_BUILD_TESTS}")
message(STATUS "Sanitizers: ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Coverage: ${VIX_ENABLE_COVERAGE}")
message(STATUS "Clang-Tidy: ${VIX_ENABLE_CLANG_TIDY}")
message(STATUS "Cppcheck: ${VIX_ENABLE_CPPCHECK}")
message(STATUS "LTO: ${VIX_ENABLE_LTO}")
message(STATUS "Install/Export enabled: ${VIX_ENABLE_INSTALL}")
message(STATUS "------------------------------------------------------")

