cmake_minimum_required(VERSION 3.20)

# ====================================================================
# Vix.cpp — Umbrella CMake Configuration
# ====================================================================
# Purpose:
#   - Build Vix modules (json, utils, core, websocket, orm, cli).
#   - Provide a single umbrella target: vix::vix
#   - Install/export a CMake package: find_package(Vix CONFIG REQUIRED)
#
# Quick start:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIX_BUILD_EXAMPLES=ON
#   cmake --build build -j
# ====================================================================

project(vix VERSION 1.16.0 LANGUAGES CXX)

# Make find_package honor *_ROOT hints (e.g. MYSQLCPPCONN_ROOT)
if (POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

# RPATH to easily run after installation
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
if (APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# ----------------------------------------------------
# Global build settings
# ----------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy compile_commands.json to repo root (tooling QoL)
add_custom_target(copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Copy compile_commands.json to project root"
)

# ----------------------------------------------------
# Options
# ----------------------------------------------------
option(VIX_ENABLE_WARNINGS     "Enable extra warnings"                    ON)
option(VIX_ENABLE_LTO          "Enable IPO/LTO (Release only)"            OFF)
option(VIX_MSVC_STATIC_RUNTIME "Link MSVC runtime statically (/MT)"       OFF)

option(VIX_ENABLE_CLANG_TIDY   "Enable clang-tidy if available"           OFF)
option(VIX_ENABLE_CPPCHECK     "Enable cppcheck if available"             OFF)

option(VIX_ENABLE_COVERAGE     "Enable coverage flags (Debug only)"       OFF)
option(VIX_ENABLE_SANITIZERS   "Enable ASan+UBSan (GCC/Clang only)"       OFF)

option(VIX_BUILD_EXAMPLES      "Build umbrella examples in ./examples"    ON)
option(VIX_BUILD_TESTS         "Build unit tests (GoogleTest)"            ON)

option(VIX_ENABLE_ORM          "Build Vix ORM module"                     ON)
option(VIX_ENABLE_WEBSOCKET    "Build Vix WebSocket module"               ON)
option(VIX_ENABLE_CLI          "Build Vix CLI module"                     ON)

option(VIX_FORCE_FETCH_JSON    "Fetch JSON backend if modules/json missing" ON)

# ----------------------------------------------------
# Tooling / Static analysis
# ----------------------------------------------------
if (VIX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if (CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "clang-tidy requested but not found")
  endif()
endif()

if (VIX_ENABLE_CPPCHECK)
  find_program(CPPCHECK_EXE NAMES cppcheck)
  if (CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK
      ${CPPCHECK_EXE}
      --enable=warning,style,performance,portability
      --std=c++20 --inline-suppr
      --suppress=unusedFunction
    )
    message(STATUS "cppcheck enabled: ${CPPCHECK_EXE}")
  else()
    message(WARNING "cppcheck requested but not found")
  endif()
endif()

# ----------------------------------------------------
# Warnings (opt-in reusable interface target)
# ----------------------------------------------------
if (VIX_ENABLE_WARNINGS)
  add_library(vix_warnings INTERFACE)
  if (MSVC)
    target_compile_options(vix_warnings INTERFACE /W4 /permissive-)
  else()
    target_compile_options(vix_warnings INTERFACE
      -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
    )
    # Silence some false positives on GCC 13+ (fmt/spdlog inlines)
    target_compile_options(vix_warnings INTERFACE
      $<$<CXX_COMPILER_ID:GNU>:-Wno-array-bounds>
    )
  endif()
endif()

# ----------------------------------------------------
# Coverage (Debug only)
# ----------------------------------------------------
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

# ----------------------------------------------------
# Sanitizers (opt-in, GCC/Clang) — as INTERFACE target
# ----------------------------------------------------
set(_VIX_SAN_COMMON "")
set(_VIX_SAN_TYPES  "")

if (VIX_ENABLE_SANITIZERS)
  if (MSVC)
    message(FATAL_ERROR "Sanitizers are not supported on MSVC. Use Clang or GCC.")
  endif()

  set(_VIX_SAN_COMMON -O1 -g -fno-omit-frame-pointer)
  set(_VIX_SAN_TYPES  -fsanitize=address,undefined)

  add_library(vix_sanitizers INTERFACE)
  target_compile_options(vix_sanitizers INTERFACE ${_VIX_SAN_COMMON} ${_VIX_SAN_TYPES})
  target_link_options(vix_sanitizers INTERFACE ${_VIX_SAN_TYPES})

  message(STATUS "Sanitizers enabled via vix_sanitizers interface target.")
endif()

# ----------------------------------------------------
# LTO (Release only)
# ----------------------------------------------------
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

# ----------------------------------------------------
# MSVC static runtime (/MT)
# ----------------------------------------------------
if (MSVC AND VIX_MSVC_STATIC_RUNTIME)
  foreach(flag_var
    CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_DEBUG
  )
    if (DEFINED ${flag_var})
      string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
  endforeach()
endif()

# ----------------------------------------------------
# Umbrella INTERFACE target (created early)
# ----------------------------------------------------
add_library(vix INTERFACE)
add_library(vix::vix ALIAS vix)

# Attach warnings/coverage to umbrella (propagates to consumers)
if (TARGET vix_warnings)
  target_link_libraries(vix INTERFACE vix_warnings)
endif()
if (TARGET vix_coverage)
  target_link_libraries(vix INTERFACE vix_coverage)
endif()

# Install include interface root
target_include_directories(vix INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ----------------------------------------------------
# Add submodules
# ----------------------------------------------------
set(_VIX_JSON_BACKEND "none")
set(JSON_TARGET "")

# --- JSON ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/json/CMakeLists.txt")
  message(STATUS "Adding 'modules/json'...")
  add_subdirectory(modules/json json_build)
  set(_VIX_JSON_BACKEND "submodule")
elseif (VIX_FORCE_FETCH_JSON)
  message(WARNING "modules/json missing — fetching nlohmann/json fallback")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)

  add_library(vix_json INTERFACE)
  target_link_libraries(vix_json INTERFACE nlohmann_json::nlohmann_json)

  if (NOT TARGET vix::json)
    add_library(vix::json ALIAS vix_json)
  endif()

  set(_VIX_JSON_BACKEND "nlohmann_json")
else()
  message(FATAL_ERROR "Missing 'modules/json'. Run: git submodule update --init --recursive")
endif()

# Resolve JSON target name
if (TARGET vix::json)
  set(JSON_TARGET vix::json)
elseif (TARGET vix_json)
  set(JSON_TARGET vix_json)
else()
  message(FATAL_ERROR "JSON backend target not found (expected vix::json or vix_json).")
endif()

# --- Utils ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/CMakeLists.txt")
  message(STATUS "Adding 'modules/utils'...")
  add_subdirectory(modules/utils utils_build)
else()
  message(FATAL_ERROR "Missing 'modules/utils'. Run: git submodule update --init --recursive")
endif()

if (TARGET spdlog::spdlog)
  get_target_property(_spdlog_inc spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
  if (_spdlog_inc)
    target_include_directories(vix::utils SYSTEM PRIVATE ${_spdlog_inc})
  endif()
endif()

if (TARGET fmt::fmt)
  get_target_property(_fmt_inc fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
  if (_fmt_inc)
    target_include_directories(vix::utils SYSTEM PRIVATE ${_fmt_inc})
  endif()
endif()

# --- Core ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/CMakeLists.txt")
  message(STATUS "Adding 'modules/core'...")
  add_subdirectory(modules/core core_build)
else()
  message(FATAL_ERROR "Missing 'modules/core'. Run: git submodule update --init --recursive")
endif()

# --- WebSocket (optional) ---
set(VIX_HAS_WEBSOCKET OFF)
if (VIX_ENABLE_WEBSOCKET AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/websocket/CMakeLists.txt")
  message(STATUS "Adding 'modules/websocket'...")
  set(VIX_WEBSOCKET_WITH_JSON "AUTO" CACHE STRING "Link JSON backend (AUTO|ON|OFF)")
  add_subdirectory(modules/websocket websocket_build)
  if (TARGET vix::websocket OR TARGET vix_websocket)
    set(VIX_HAS_WEBSOCKET ON)
  endif()
else()
  message(STATUS "WebSocket: disabled or not present.")
endif()

# --- CLI (optional) ---
if (VIX_ENABLE_CLI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cli/CMakeLists.txt")
  message(STATUS "Adding 'modules/cli'...")
  add_subdirectory(modules/cli cli_build)
else()
  message(STATUS "CLI: disabled or not present.")
endif()

# --- ORM (optional) ---
# Forwarded to modules/orm (users can override via -D flags)
option(VIX_ORM_USE_MYSQL "Enable MySQL backend in vix_orm" ON)
set(VIX_ORM_BUILD_EXAMPLES OFF CACHE BOOL "Build vix_orm examples")
set(VIX_ORM_BUILD_TESTS    OFF CACHE BOOL "Build vix_orm tests")

set(VIX_UMBRELLA_BUILD ON CACHE BOOL "Building Vix from umbrella" FORCE)
set(VIX_HAS_ORM OFF)
if (VIX_ENABLE_ORM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/orm/CMakeLists.txt")
  message(STATUS "Adding 'modules/orm'...")
  add_subdirectory(modules/orm orm_build)

  if (TARGET vix::orm OR TARGET vix_orm)
    if (TARGET vix_orm AND NOT TARGET vix::orm)
      add_library(vix::orm ALIAS vix_orm)
    endif()
    set(VIX_HAS_ORM ON)
  else()
    message(WARNING "ORM module added but no vix::orm target was exported.")
  endif()
else()
  message(STATUS "ORM: disabled or not present.")
endif()

# ----------------------------------------------------
# Link umbrella to required module targets
# ----------------------------------------------------
target_link_libraries(vix INTERFACE
  vix::core
  vix::utils
  ${JSON_TARGET}
)

# Link websocket only if it exists
if (TARGET vix::websocket)
  target_link_libraries(vix INTERFACE vix::websocket)
elseif (TARGET vix_websocket)
  add_library(vix::websocket ALIAS vix_websocket)
  target_link_libraries(vix INTERFACE vix::websocket)
endif()

# Propagate sanitizers to consumers (umbrella)
if (TARGET vix_sanitizers)
  target_link_libraries(vix INTERFACE vix_sanitizers)
endif()

# ----------------------------------------------------
# Examples filtering
# ----------------------------------------------------
# Examples that require ORM at all
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

# Subset of ORM examples that require MySQL backend
set(_ORM_MYSQL_EXAMPLES
  demo_with_orm
  error_handling
  batch_insert_tx
  users_crud
  repository_crud_full
  migrate_init
  tx_unit_of_work
  querybuilder_update
)

# ----------------------------------------------------
# Examples (flat: examples/*.cpp and examples/orm/*.cpp)
# ----------------------------------------------------
if (VIX_BUILD_EXAMPLES)
  file(GLOB VIX_UMBRELLA_EXAMPLES
       CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
       "${CMAKE_CURRENT_SOURCE_DIR}/examples/orm/*.cpp")

  # 1) If ORM is OFF → remove all ORM examples
  if (NOT VIX_HAS_ORM)
    foreach(_ex IN LISTS _ORM_EXAMPLES)
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*/${_ex}\\.cpp$")
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*\\\\${_ex}\\.cpp$")
    endforeach()
    message(STATUS "Examples: ORM OFF → filtered ORM examples: ${_ORM_EXAMPLES}")
  endif()

  # 2) If ORM is ON but MySQL backend is OFF → remove MySQL-based ORM examples
  if (VIX_HAS_ORM AND NOT VIX_ORM_USE_MYSQL)
    foreach(_ex IN LISTS _ORM_MYSQL_EXAMPLES)
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*/${_ex}\\.cpp$")
      list(FILTER VIX_UMBRELLA_EXAMPLES EXCLUDE REGEX ".*\\\\${_ex}\\.cpp$")
    endforeach()
    message(STATUS "Examples: MySQL OFF → filtered MySQL ORM examples: ${_ORM_MYSQL_EXAMPLES}")
  endif()

  if (VIX_UMBRELLA_EXAMPLES)
    foreach(EXAMPLE_SRC IN LISTS VIX_UMBRELLA_EXAMPLES)
      get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)

      if (_raw_name MATCHES "_common$" OR _raw_name MATCHES "_disabled$" OR _raw_name MATCHES "_internal$")
        message(STATUS "Skipping helper/internal example: ${_raw_name}")
        continue()
      endif()

      set(EXAMPLE_NAME "${_raw_name}")
      if (TARGET "${EXAMPLE_NAME}")
        set(EXAMPLE_NAME "ex_${_raw_name}")
        message(WARNING "Target '${_raw_name}' already exists. Using '${EXAMPLE_NAME}'.")
      endif()

      add_executable("${EXAMPLE_NAME}" "${EXAMPLE_SRC}")
      target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix::vix)

      # ✅ Link ORM only for examples/orm/*
      if (VIX_HAS_ORM AND TARGET vix::orm AND EXAMPLE_SRC MATCHES "examples[/\\\\]orm[/\\\\]")
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix::orm)
      endif()

      if (TARGET vix_warnings)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_warnings)
      endif()

      if (TARGET vix_sanitizers)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_sanitizers)
      endif()

      if (TARGET vix_coverage)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_coverage)
      endif()

      set_target_properties("${EXAMPLE_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
      )

      message(STATUS "Example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
    endforeach()
  else()
    message(STATUS "No example sources found under examples/.")
  endif()
endif()

# ----------------------------------------------------
# Grouped examples (subdirectories) — optional
# ----------------------------------------------------
if (VIX_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
  message(STATUS "Examples: enabling grouped examples via examples/CMakeLists.txt")
  add_subdirectory(examples)
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

  file(GLOB_RECURSE VIX_TEST_SOURCES CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp")

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
    message(STATUS "No *_test.cpp found under tests/ — skipping.")
  endif()
endif()

# ----------------------------------------------------
# Runtime config copy (optional)
# ----------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/config.json")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/config.json"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# ----------------------------------------------------
# Install/export (find_package(Vix CONFIG))
# ----------------------------------------------------
option(VIX_ENABLE_INSTALL "Enable install/export rules for packaging" ON)

# Compute feature flags as real ON/OFF at configure-time (NO generator expressions)
set(VIX_WITH_JSON     ON)     # umbrella always links JSON backend
set(VIX_WITH_BOOST_FS ON)     # adjust if you ever disable Boost::filesystem in core
set(VIX_WITH_OPENSSL  ON)     # adjust if you ever disable SSL in core
set(VIX_WITH_MYSQL    OFF)    # do NOT require MySQL for consumers by default
set(VIX_WITH_SQLITE   OFF)    # will be set ON only if websocket exists

if (TARGET vix::websocket OR TARGET vix_websocket)
  set(VIX_WITH_SQLITE ON)
endif()

if (VIX_ENABLE_INSTALL)
  # Install headers if present
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include")
    install(DIRECTORY modules/core/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/include")
    install(DIRECTORY modules/utils/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/json/include")
    install(DIRECTORY modules/json/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/websocket/include")
    install(DIRECTORY modules/websocket/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (VIX_HAS_ORM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/orm/include")
    install(DIRECTORY modules/orm/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  # Export umbrella target (modules must also export their targets into VixTargets)
  install(TARGETS vix EXPORT VixTargets)

  if (TARGET vix_warnings)
    install(TARGETS vix_warnings EXPORT VixTargets)
  endif()

  if (TARGET vix_coverage)
    install(TARGETS vix_coverage EXPORT VixTargets)
  endif()

  # Config files
  set(VIX_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Vix")

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Export flags for consumers (substituted into VixConfig.cmake)
  set(VIX_HAS_ORM ${VIX_HAS_ORM})                 # already computed
  set(VIX_ORM_WITH_MYSQL OFF)
  if (VIX_HAS_ORM AND VIX_ORM_USE_MYSQL)
    set(VIX_ORM_WITH_MYSQL ON)
  endif()

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VixConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
    INSTALL_DESTINATION "${VIX_INSTALL_CMAKEDIR}"
  )

  install(EXPORT VixTargets
          NAMESPACE vix::
          DESTINATION "${VIX_INSTALL_CMAKEDIR}")

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
    DESTINATION "${VIX_INSTALL_CMAKEDIR}"
  )

  # Install CLI executable (not exported)
  if (TARGET vix_cli)
    install(TARGETS vix_cli
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            RENAME vix)
    message(STATUS "CLI install: installing target 'vix_cli' as 'vix'")
  else()
    message(STATUS "CLI install: no 'vix_cli' target detected — CLI not installed")
  endif()
endif()

# ----------------------------------------------------
# Summary
# ----------------------------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "Vix umbrella configured")
message(STATUS "Project version   : ${PROJECT_VERSION}")
message(STATUS "JSON backend      : ${_VIX_JSON_BACKEND}")
message(STATUS "WebSocket built   : ${VIX_HAS_WEBSOCKET}")
message(STATUS "ORM packaged      : ${VIX_HAS_ORM}")
message(STATUS "Examples          : ${VIX_BUILD_EXAMPLES}")
message(STATUS "Tests             : ${VIX_BUILD_TESTS}")
message(STATUS "Sanitizers        : ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Coverage          : ${VIX_ENABLE_COVERAGE}")
message(STATUS "LTO               : ${VIX_ENABLE_LTO}")
message(STATUS "Install/Export    : ${VIX_ENABLE_INSTALL}")
message(STATUS "Flags exported    : JSON=${VIX_WITH_JSON} BOOST_FS=${VIX_WITH_BOOST_FS} OPENSSL=${VIX_WITH_OPENSSL} SQLITE=${VIX_WITH_SQLITE} MYSQL=${VIX_WITH_MYSQL} HAS_ORM=${VIX_HAS_ORM}")
message(STATUS "------------------------------------------------------")

