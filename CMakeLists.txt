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

# Resolve umbrella version from git (preferred)
set(_VIX_VERSION_FALLBACK "0.0.0")

find_package(Git QUIET)
set(_VIX_GIT_DESCRIBE "")
if(Git_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE _VIX_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()

# Compute PROJECT_VERSION (must be numeric for CMake packaging)
# If describe returns: v1.20.1-4-gXXXX -> numeric is 1.20.1
set(_VIX_PROJECT_VERSION "${_VIX_VERSION_FALLBACK}")
if(_VIX_GIT_DESCRIBE MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)")
  set(_VIX_PROJECT_VERSION "${CMAKE_MATCH_1}")
endif()

project(vix VERSION ${_VIX_PROJECT_VERSION} LANGUAGES CXX)

# Human-friendly version string (can include -N-gHASH[-dirty])
if(_VIX_GIT_DESCRIBE STREQUAL "")
  set(VIX_UMBRELLA_VERSION "v${PROJECT_VERSION}")
else()
  set(VIX_UMBRELLA_VERSION "${_VIX_GIT_DESCRIBE}")
endif()
set(VIX_UMBRELLA_VERSION "${VIX_UMBRELLA_VERSION}" CACHE STRING "Umbrella version" FORCE)

set(VIX_UMBRELLA_BUILD ON CACHE BOOL "Building Vix from umbrella" FORCE)

# Make find_package honor *_ROOT hints (e.g. MYSQLCPPCONN_ROOT)
if (POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

# Resolve to a real target (not an ALIAS) so we can mutate properties safely.
function(vix_resolve_real_target out name)
  set(_real "")
  if (TARGET "${name}")
    get_target_property(_aliased "${name}" ALIASED_TARGET)
    if (_aliased)
      set(_real "${_aliased}")
    else()
      set(_real "${name}")
    endif()
  endif()
  set(${out} "${_real}" PARENT_SCOPE)
endfunction()

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
          "${CMAKE_BINARY_DIR}/compile_commands.json"
          "${CMAKE_SOURCE_DIR}/compile_commands.json"
  COMMENT "Copy compile_commands.json to project root"
)

# ----------------------------------------------------
# Options
# ----------------------------------------------------
option(VIX_ENABLE_WARNINGS         "Enable extra warnings"                                        ON)
option(VIX_ENABLE_LTO              "Enable IPO/LTO (Release only)"                                OFF)
option(VIX_MSVC_STATIC_RUNTIME     "Link MSVC runtime statically (/MT)"                           OFF)
option(VIX_ENABLE_CLANG_TIDY       "Enable clang-tidy if available"                               OFF)
option(VIX_ENABLE_CPPCHECK         "Enable cppcheck if available"                                 OFF)
option(VIX_ENABLE_COVERAGE         "Enable coverage flags (Debug only)"                           OFF)
option(VIX_ENABLE_SANITIZERS       "Enable ASan+UBSan (GCC/Clang only)"                           OFF)
option(VIX_BUILD_EXAMPLES          "Build umbrella examples in ./examples"                        ON)
option(VIX_BUILD_TESTS             "Build unit tests (GoogleTest)"                                ON)
option(VIX_ENABLE_ORM              "Build Vix ORM module"                                         ON)
option(VIX_ENABLE_WEBSOCKET        "Build Vix WebSocket module"                                   ON)
option(VIX_ENABLE_CLI              "Build Vix CLI module"                                         ON)
option(VIX_FORCE_FETCH_JSON        "Fetch JSON backend if modules/json missing"                   ON)
option(VIX_BENCH_MODE              "Disable heavy security/logging checks for benchmarks"         OFF)
option(VIX_ENABLE_MIDDLEWARE       "Build Vix middleware module"                                  ON)
option(VIX_ENABLE_HTTP_COMPRESSION "Enable HTTP compression middleware deps (zlib/brotli)"        ON)
option(VIX_ENABLE_DB               "Build Vix DB module (core anti-ORM)"                          ON)
option(VIX_DB_USE_MYSQL            "Enable MySQL backend in vix_db"                               ON)
option(VIX_DB_USE_SQLITE           "Enable SQLite backend in vix_db"                              OFF)
option(VIX_DB_USE_POSTGRES         "Enable PostgreSQL backend in vix_db"                          OFF)
option(VIX_DB_USE_REDIS            "Enable Redis backend in vix_db"                               OFF)
option(VIX_ENABLE_P2P              "Build Vix P2P module"                                         ON)
option(VIX_ENABLE_P2P_HTTP         "Build Vix P2P HTTP adapter module"                            ON)
option(VIX_ENABLE_CACHE            "Build Vix Cache module"                                       ON)
option(VIX_FETCH_DEPS              "Allow fetching missing deps from the internet"                OFF)
option(VIX_ENABLE_ASYNC            "Build Vix Async module"                                       ON)
option(VIX_ENABLE_VALIDATION       "Build Vix Validation module"                                  ON)
option(VIX_ENABLE_CRYPTO           "Build Vix Crypto module"                                      ON)
option(VIX_ENABLE_WEBRPC           "Build Vix WebRPC module"                                      ON)

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
# ----------------------------------------------------
# Third-party: Asio (standalone, header-only)
# Exposes: vix::thirdparty_asio
# ----------------------------------------------------
add_library(vix_thirdparty_asio INTERFACE)
add_library(vix::thirdparty_asio ALIAS vix_thirdparty_asio)
target_compile_definitions(vix_thirdparty_asio INTERFACE ASIO_STANDALONE=1)

set(VIX_THIRDPARTY_ASIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio-src/asio/include")

if (EXISTS "${VIX_THIRDPARTY_ASIO_DIR}/asio.hpp")
  message(STATUS "Asio: using vendored Asio at ${VIX_THIRDPARTY_ASIO_DIR}")
  target_include_directories(vix_thirdparty_asio SYSTEM INTERFACE
    $<BUILD_INTERFACE:${VIX_THIRDPARTY_ASIO_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vix/third_party/asio>
  )
else()
  message(FATAL_ERROR "Asio submodule missing: third_party/asio-src. Run: git submodule update --init --recursive")
endif()
target_link_libraries(vix INTERFACE vix::thirdparty_asio)

add_library(vix::vix ALIAS vix)

# ----------------------------------------------------
# Optional deps: HTTP compression (zlib + brotli)
# Propagate defs/libs to all consumers through vix::vix
# ----------------------------------------------------
if (VIX_ENABLE_HTTP_COMPRESSION)
  # ---- zlib (gzip) ----
  find_package(ZLIB QUIET)
  if (ZLIB_FOUND)
    message(STATUS "Compression: zlib found -> gzip enabled")
    target_compile_definitions(vix INTERFACE VIX_HAS_ZLIB=1)
    target_link_libraries(vix INTERFACE ZLIB::ZLIB)
  else()
    message(STATUS "Compression: zlib not found -> gzip disabled")
  endif()

  # ---- brotli (br) ----
  find_library(BROTLI_ENC NAMES brotlienc)
  find_library(BROTLI_COMMON NAMES brotlicommon)

  if (BROTLI_ENC AND BROTLI_COMMON)
    message(STATUS "Compression: brotli found -> br enabled")
    target_compile_definitions(vix INTERFACE VIX_HAS_BROTLI=1)
    target_link_libraries(vix INTERFACE ${BROTLI_ENC} ${BROTLI_COMMON})
  else()
    message(STATUS "Compression: brotli not found -> br disabled")
  endif()
endif()


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

# --- WebRPC (optional) ---
set(VIX_HAS_WEBRPC OFF)

if (VIX_ENABLE_WEBRPC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/webrpc/CMakeLists.txt")
  message(STATUS "Adding 'modules/webrpc'...")
  add_subdirectory(modules/webrpc webrpc_build)

  # Normalise: expose vix::webrpc
  if (TARGET vix::webrpc OR TARGET vix_webrpc)
    set(VIX_HAS_WEBRPC ON)
    if (TARGET vix_webrpc AND NOT TARGET vix::webrpc)
      add_library(vix::webrpc ALIAS vix_webrpc)
    endif()
  else()
    message(WARNING "WebRPC module added but no vix::webrpc target was exported.")
  endif()
else()
  message(STATUS "WebRPC: disabled or not present.")
endif()

# --- Utils ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/CMakeLists.txt")
  message(STATUS "Adding 'modules/utils'...")
  add_subdirectory(modules/utils utils_build)
else()
  message(FATAL_ERROR "Missing 'modules/utils'. Run: git submodule update --init --recursive")
endif()

# --- Crypto (optional) ---
set(VIX_HAS_CRYPTO OFF)

if (VIX_ENABLE_CRYPTO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/crypto/CMakeLists.txt")
  message(STATUS "Adding 'modules/crypto'...")

  # Important: en umbrella, crypto ne fait pas find_package(OpenSSL),
  # il attend que OpenSSL::Crypto existe deja.
  find_package(OpenSSL QUIET)
  if (OpenSSL_FOUND AND TARGET OpenSSL::Crypto)
    message(STATUS "Crypto: OpenSSL found -> provider available")
  else()
    message(STATUS "Crypto: OpenSSL not found -> provider will be disabled")
  endif()

  add_subdirectory(modules/crypto crypto_build)

  if (TARGET vix::crypto OR TARGET vix_crypto)
    set(VIX_HAS_CRYPTO ON)
    if (TARGET vix_crypto AND NOT TARGET vix::crypto)
      add_library(vix::crypto ALIAS vix_crypto)
    endif()
  else()
    message(WARNING "Crypto module added but no vix::crypto target was exported.")
  endif()
else()
  message(STATUS "Crypto: disabled or not present.")
endif()


# Resolve real utils target (dereference ALIAS if needed)
set(_VIX_UTILS_REAL "")

vix_resolve_real_target(_VIX_UTILS_REAL vix_utils)
if (NOT _VIX_UTILS_REAL)
  vix_resolve_real_target(_VIX_UTILS_REAL vix-utils)
endif()
if (NOT _VIX_UTILS_REAL)
  vix_resolve_real_target(_VIX_UTILS_REAL utils)
endif()

# Propagate transitive include dirs from deps (avoid warnings from 3rd-party headers)
if (_VIX_UTILS_REAL AND NOT _VIX_UTILS_REAL STREQUAL "")
  if (TARGET spdlog::spdlog)
    get_target_property(_spdlog_inc spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
    if (_spdlog_inc)
      target_include_directories(${_VIX_UTILS_REAL} SYSTEM INTERFACE ${_spdlog_inc})
    endif()
  endif()

  if (TARGET fmt::fmt)
    get_target_property(_fmt_inc fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    if (_fmt_inc)
      target_include_directories(${_VIX_UTILS_REAL} SYSTEM INTERFACE ${_fmt_inc})
    endif()
  endif()
else()
  message(WARNING "Could not resolve real utils target (tried: vix_utils, vix-utils, utils).")
endif()

# --- Core ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/core/CMakeLists.txt")
  message(STATUS "Adding 'modules/core'...")
  add_subdirectory(modules/core core_build)
else()
  message(FATAL_ERROR "Missing 'modules/core'. Run: git submodule update --init --recursive")
endif()

# --- Conversion (required by Validation) ---
set(VIX_HAS_CONVERSION OFF)

if (TARGET vix::conversion)
  set(VIX_HAS_CONVERSION ON)
else()
  # If you have it as a module, add it here (umbrella must not fetch)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/conversion/CMakeLists.txt")
    message(STATUS "Adding 'modules/conversion'...")
    add_subdirectory(modules/conversion conversion_build)

    if (TARGET vix::conversion OR TARGET vix_conversion)
      set(VIX_HAS_CONVERSION ON)
      if (TARGET vix_conversion AND NOT TARGET vix::conversion)
        add_library(vix::conversion ALIAS vix_conversion)
      endif()
    else()
      message(FATAL_ERROR "Conversion module added but no vix::conversion target was exported.")
    endif()
  else()
    message(STATUS "Conversion: not present as module. Expecting it to be provided by core or another module.")
  endif()
endif()

# --- Validation (optional) ---
set(VIX_HAS_VALIDATION OFF)

if (VIX_ENABLE_VALIDATION AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/validation/CMakeLists.txt")
  if (NOT TARGET vix::conversion)
    message(FATAL_ERROR "Validation requires vix::conversion. Ensure conversion is added before validation in the umbrella.")
  endif()

  message(STATUS "Adding 'modules/validation'...")
  add_subdirectory(modules/validation validation_build)

  if (TARGET vix::validation OR TARGET vix_validation)
    set(VIX_HAS_VALIDATION ON)
    if (TARGET vix_validation AND NOT TARGET vix::validation)
      add_library(vix::validation ALIAS vix_validation)
    endif()
  else()
    message(WARNING "Validation module added but no vix::validation target was exported.")
  endif()
else()
  message(STATUS "Validation: disabled or not present.")
endif()

# --- Async (optional) ---
set(VIX_HAS_ASYNC OFF)
if (VIX_ENABLE_ASYNC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/async/CMakeLists.txt")
  message(STATUS "Adding 'modules/async'...")
  add_subdirectory(modules/async async_build)

  if (TARGET vix::async OR TARGET vix_async)
    set(VIX_HAS_ASYNC ON)
    if (TARGET vix_async AND NOT TARGET vix::async)
      add_library(vix::async ALIAS vix_async)
    endif()
  else()
    message(WARNING "Async module added but no vix::async target was exported.")
  endif()
else()
  message(STATUS "Async: disabled or not present.")
endif()

# --- Net (required by P2P) ---
set(VIX_HAS_NET OFF)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/net/CMakeLists.txt")
  message(STATUS "Adding 'modules/net'...")
  add_subdirectory(modules/net net_build)

  if (TARGET vix::net OR TARGET vix_net)
    set(VIX_HAS_NET ON)
    if (TARGET vix_net AND NOT TARGET vix::net)
      add_library(vix::net ALIAS vix_net)
    endif()
  else()
    message(WARNING "Net module added but no vix::net target was exported.")
  endif()
else()
  message(STATUS "Net: not present (P2P will be disabled or will fetch standalone).")
endif()

# --- Cache (optional, used by P2P) ---
set(VIX_HAS_CACHE OFF)
if (VIX_ENABLE_CACHE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cache/CMakeLists.txt")
  message(STATUS "Adding 'modules/cache'...")
  add_subdirectory(modules/cache cache_build)

  if (TARGET vix::cache OR TARGET vix_cache)
    set(VIX_HAS_CACHE ON)
    if (TARGET vix_cache AND NOT TARGET vix::cache)
      add_library(vix::cache ALIAS vix_cache)
    endif()
  else()
    message(WARNING "Cache module added but no vix::cache target was exported.")
  endif()
else()
  message(STATUS "Cache: disabled or not present.")
endif()

# --- Sync (optional, used by P2P) ---
set(VIX_HAS_SYNC OFF)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/sync/CMakeLists.txt")
  message(STATUS "Adding 'modules/sync'...")
  add_subdirectory(modules/sync sync_build)

  if (TARGET vix::sync OR TARGET vix_sync)
    set(VIX_HAS_SYNC ON)
    if (TARGET vix_sync AND NOT TARGET vix::sync)
      add_library(vix::sync ALIAS vix_sync)
    endif()
  endif()
else()
  message(STATUS "Sync: not present.")
endif()

# If net is missing, P2P must be disabled in umbrella (no auto-fetch here)
if (VIX_ENABLE_P2P AND NOT VIX_HAS_NET)
  message(WARNING "P2P requested but Net module is missing -> disabling P2P in umbrella")
  set(VIX_ENABLE_P2P OFF CACHE BOOL "Build Vix P2P module" FORCE)
endif()

# --- P2P (optional) ---
set(VIX_HAS_P2P OFF)
if (VIX_ENABLE_P2P AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/p2p/CMakeLists.txt")
  message(STATUS "Adding 'modules/p2p'...")
  add_subdirectory(modules/p2p p2p_build)

  # Normalise: expose vix::p2p
  if (TARGET vix::p2p OR TARGET vix_p2p)
    set(VIX_HAS_P2P ON)
    if (TARGET vix_p2p AND NOT TARGET vix::p2p)
      add_library(vix::p2p ALIAS vix_p2p)
    endif()
  else()
    message(WARNING "P2P module added but no vix::p2p target was exported.")
  endif()
else()
  message(STATUS "P2P: disabled or not present.")
endif()

# --- P2P HTTP Adapter (optional) ---
set(VIX_HAS_P2P_HTTP OFF)

# Guard: p2p_http requires both core + p2p
if (VIX_ENABLE_P2P_HTTP AND VIX_HAS_P2P AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/p2p_http/CMakeLists.txt")
  message(STATUS "Adding 'modules/p2p_http'...")
  add_subdirectory(modules/p2p_http p2p_http_build)

  if (TARGET vix::p2p_http OR TARGET vix_p2p_http)
    set(VIX_HAS_P2P_HTTP ON)
    if (TARGET vix_p2p_http AND NOT TARGET vix::p2p_http)
      add_library(vix::p2p_http ALIAS vix_p2p_http)
    endif()
  else()
    message(WARNING "p2p_http module added but no vix::p2p_http target was exported.")
  endif()
else()
  message(STATUS "P2P HTTP: disabled, missing, or P2P not built.")
endif()

# --- Middleware (optional) ---
set(VIX_HAS_MIDDLEWARE OFF)
if (VIX_ENABLE_MIDDLEWARE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/middleware/CMakeLists.txt")
  message(STATUS "Adding 'modules/middleware'...")
  add_subdirectory(modules/middleware middleware_build)

  # Normalise: expose vix::middleware
  if (TARGET vix::middleware OR TARGET vix_middleware)
    set(VIX_HAS_MIDDLEWARE ON)
    if (TARGET vix_middleware AND NOT TARGET vix::middleware)
      add_library(vix::middleware ALIAS vix_middleware)
    endif()
  else()
    message(WARNING "Middleware module added but no vix::middleware target was exported.")
  endif()
else()
  message(STATUS "Middleware: disabled or not present.")
endif()

# Bench mode (compile-time flags)
if (VIX_BENCH_MODE)
  message(STATUS "Bench mode enabled: defining VIX_BENCH_MODE")

  # Resolve the real core target (vix::core is an ALIAS in your tree)
  set(_VIX_CORE_REAL "")

  set(_VIX_CORE_REAL "")
  vix_resolve_real_target(_VIX_CORE_REAL vix_core)
  if (NOT _VIX_CORE_REAL)
    vix_resolve_real_target(_VIX_CORE_REAL vix-core)
  endif()
  if (NOT _VIX_CORE_REAL)
    vix_resolve_real_target(_VIX_CORE_REAL core)
  endif()

  if (_VIX_CORE_REAL)
    target_compile_definitions(${_VIX_CORE_REAL} PUBLIC VIX_BENCH_MODE=1)
  else()
    message(WARNING "Bench mode requested but could not find real core target (tried: vix_core, vix-core, core).")
  endif()
endif()

# --- DB (optional, required by ORM) ---
set(VIX_HAS_DB OFF)
if (VIX_ENABLE_DB AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/db/CMakeLists.txt")
  message(STATUS "Adding 'modules/db'...")
  add_subdirectory(modules/db db_build)

  if (TARGET vix::db OR TARGET vix_db)
    if (TARGET vix_db AND NOT TARGET vix::db)
      add_library(vix::db ALIAS vix_db)
    endif()
    set(VIX_HAS_DB ON)
  else()
    message(WARNING "DB module added but no vix::db target was exported.")
  endif()
else()
  message(STATUS "DB: disabled or not present.")
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
set(VIX_HAS_ORM OFF)
if (VIX_ENABLE_ORM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/orm/CMakeLists.txt")

  if (NOT VIX_HAS_DB)
    message(FATAL_ERROR "ORM requires DB module. Enable it with -DVIX_ENABLE_DB=ON and ensure modules/db exists.")
  endif()

  message(STATUS "Adding 'modules/orm'...")
  set(VIX_ORM_BUILD_EXAMPLES OFF CACHE BOOL "Build vix_orm examples")
  set(VIX_ORM_BUILD_TESTS    OFF CACHE BOOL "Build vix_orm tests")

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

if (TARGET vix::conversion)
  target_link_libraries(vix INTERFACE vix::conversion)
endif()

if (TARGET vix::validation)
  target_link_libraries(vix INTERFACE vix::validation)
endif()

if (TARGET vix::crypto)
  target_link_libraries(vix INTERFACE vix::crypto)
endif()

if (TARGET vix::net)
  target_link_libraries(vix INTERFACE vix::net)
endif()

if (TARGET vix::cache)
  target_link_libraries(vix INTERFACE vix::cache)
endif()

if (TARGET vix::sync)
  target_link_libraries(vix INTERFACE vix::sync)
endif()

if (TARGET vix::async)
  target_link_libraries(vix INTERFACE vix::async)
endif()

if (TARGET vix::p2p)
  target_link_libraries(vix INTERFACE vix::p2p)
endif()

if (TARGET vix::p2p_http)
  target_link_libraries(vix INTERFACE vix::p2p_http)
endif()

if (TARGET vix::db)
  target_link_libraries(vix INTERFACE vix::db)
endif()

if (TARGET vix::webrpc)
  target_link_libraries(vix INTERFACE vix::webrpc)
endif()

# Link websocket only if it exists
if (TARGET vix::websocket)
  target_link_libraries(vix INTERFACE vix::websocket)
elseif (TARGET vix_websocket)
  add_library(vix::websocket ALIAS vix_websocket)
  target_link_libraries(vix INTERFACE vix::websocket)
endif()

# Link middleware only if it exists
if (TARGET vix::middleware)
  target_link_libraries(vix INTERFACE vix::middleware)
  target_compile_definitions(vix INTERFACE VIX_HAS_MIDDLEWARE=1)
elseif (TARGET vix_middleware)
  add_library(vix::middleware ALIAS vix_middleware)
  target_link_libraries(vix INTERFACE vix::middleware)
  target_compile_definitions(vix INTERFACE VIX_HAS_MIDDLEWARE=1)
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
 if (VIX_HAS_ORM AND NOT VIX_DB_USE_MYSQL)
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
set(VIX_WITH_BOOST_FS OFF)    # do NOT require Boost for consumers by default
set(VIX_WITH_OPENSSL  ON)     # adjust if you ever disable SSL in core
set(VIX_WITH_MYSQL    OFF)    # do NOT require MySQL for consumers by default
set(VIX_WITH_SQLITE   OFF)    # will be set ON only if websocket exists

set(VIX_WITH_ZLIB OFF)
if (VIX_ENABLE_HTTP_COMPRESSION AND ZLIB_FOUND)
  set(VIX_WITH_ZLIB ON)
endif()

set(VIX_WITH_MYSQL OFF)
if (VIX_HAS_DB AND VIX_DB_USE_MYSQL)
  set(VIX_WITH_MYSQL ON)
endif()

set(VIX_WITH_CACHE OFF)
if (VIX_HAS_CACHE)
  set(VIX_WITH_CACHE ON)
endif()

set(VIX_WITH_P2P OFF)
if (VIX_HAS_P2P)
  set(VIX_WITH_P2P ON)
endif()

set(VIX_WITH_P2P_HTTP OFF)
if (VIX_HAS_P2P_HTTP)
  set(VIX_WITH_P2P_HTTP ON)
endif()

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

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/crypto/include")
    install(DIRECTORY modules/crypto/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/conversion/include")
    install(DIRECTORY modules/conversion/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/validation/include")
    install(DIRECTORY modules/validation/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/json/include")
    install(DIRECTORY modules/json/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/webrpc/include")
    install(DIRECTORY modules/webrpc/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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

  if (VIX_HAS_DB AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/db/include")
    install(DIRECTORY modules/db/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/async/include")
    install(DIRECTORY modules/async/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/net/include")
    install(DIRECTORY modules/net/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/cache/include")
    install(DIRECTORY modules/cache/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/sync/include")
    install(DIRECTORY modules/sync/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (VIX_HAS_P2P AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/p2p/include")
    install(DIRECTORY modules/p2p/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (VIX_HAS_P2P_HTTP AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/p2p_http/include")
    install(DIRECTORY modules/p2p_http/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/middleware/include")
    install(DIRECTORY modules/middleware/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
  endif()

    # ---- Install Asio (submodule) — robust (no symlink issues) ----
  set(_VIX_ASIO_INSTALL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio-src/asio/include")

  if (NOT EXISTS "${_VIX_ASIO_INSTALL_SRC}/asio.hpp")
    message(FATAL_ERROR "Asio submodule missing: third_party/asio-src. Run: git submodule update --init --recursive")
  endif()

  file(GLOB_RECURSE _VIX_ASIO_HEADERS
    CONFIGURE_DEPENDS
    "${_VIX_ASIO_INSTALL_SRC}/*.hpp"
    "${_VIX_ASIO_INSTALL_SRC}/*.h"
  )

  install(FILES ${_VIX_ASIO_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vix/third_party/asio"
  )

  install(TARGETS vix EXPORT VixTargets)

  if (TARGET vix_thirdparty_asio)
    install(TARGETS vix_thirdparty_asio EXPORT VixTargets)
  endif()

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

  set(VIX_HAS_ORM ${VIX_HAS_ORM})

  set(VIX_DB_WITH_MYSQL OFF)
  if (VIX_HAS_DB AND VIX_DB_USE_MYSQL)
    set(VIX_DB_WITH_MYSQL ON)
  endif()

  set(VIX_HAS_CACHE  ${VIX_HAS_CACHE})
  set(VIX_HAS_P2P    ${VIX_HAS_P2P})
  set(VIX_WITH_CACHE ${VIX_WITH_CACHE})
  set(VIX_WITH_P2P   ${VIX_WITH_P2P})
  set(VIX_HAS_P2P_HTTP  ${VIX_HAS_P2P_HTTP})
  set(VIX_WITH_P2P_HTTP ${VIX_WITH_P2P_HTTP})

  set(VIX_WITH_CONVERSION OFF)
  if (VIX_HAS_CONVERSION)
    set(VIX_WITH_CONVERSION ON)
  endif()

  set(VIX_WITH_CRYPTO OFF)
  if (VIX_HAS_CRYPTO)
    set(VIX_WITH_CRYPTO ON)
  endif()

  set(VIX_HAS_CRYPTO  ${VIX_HAS_CRYPTO})
  set(VIX_WITH_CRYPTO ${VIX_WITH_CRYPTO})

  set(VIX_WITH_WEBRPC OFF)
  if (VIX_HAS_WEBRPC)
    set(VIX_WITH_WEBRPC ON)
  endif()

  set(VIX_WITH_VALIDATION OFF)
  if (VIX_HAS_VALIDATION)
    set(VIX_WITH_VALIDATION ON)
  endif()

  set(VIX_HAS_VALIDATION  ${VIX_HAS_VALIDATION})
  set(VIX_WITH_VALIDATION ${VIX_WITH_VALIDATION})

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
    )
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
message(STATUS "WebRPC built      : ${VIX_HAS_WEBRPC}")
message(STATUS "WebSocket built   : ${VIX_HAS_WEBSOCKET}")
message(STATUS "ORM packaged      : ${VIX_HAS_ORM}")
message(STATUS "DB built         : ${VIX_HAS_DB}")
message(STATUS "P2P built         : ${VIX_HAS_P2P}")
message(STATUS "Conversion built : ${VIX_HAS_CONVERSION}")
message(STATUS "Validation built : ${VIX_HAS_VALIDATION}")
message(STATUS "Middleware built : ${VIX_HAS_MIDDLEWARE}")
message(STATUS "Examples          : ${VIX_BUILD_EXAMPLES}")
message(STATUS "Tests             : ${VIX_BUILD_TESTS}")
message(STATUS "Sanitizers        : ${VIX_ENABLE_SANITIZERS}")
message(STATUS "Coverage          : ${VIX_ENABLE_COVERAGE}")
message(STATUS "LTO               : ${VIX_ENABLE_LTO}")
message(STATUS "Install/Export    : ${VIX_ENABLE_INSTALL}")
message(STATUS "Flags exported    : JSON=${VIX_WITH_JSON} CONVERSION=${VIX_WITH_CONVERSION} VALIDATION=${VIX_WITH_VALIDATION} WEBRPC=${VIX_WITH_WEBRPC} CRYPTO=${VIX_WITH_CRYPTO} BOOST_FS=${VIX_WITH_BOOST_FS} OPENSSL=${VIX_WITH_OPENSSL} SQLITE=${VIX_WITH_SQLITE} MYSQL=${VIX_WITH_MYSQL} HAS_ORM=${VIX_HAS_ORM} CACHE=${VIX_WITH_CACHE} P2P=${VIX_WITH_P2P}")
message(STATUS "------------------------------------------------------")

