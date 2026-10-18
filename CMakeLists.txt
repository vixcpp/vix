cmake_minimum_required(VERSION 3.23)

# ====================================================================
# Vix.cpp — Umbrella CMake Configuration
# ====================================================================
# Purpose:
#   - Build the admitted Vix Standard, Files, Process, Network, and Concurrency surfaces.
#   - Install/export the canonical package: find_package(Vix CONFIG REQUIRED)
#
# Quick start:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIX_BUILD_EXAMPLES=ON
#   cmake --build build -j
# ====================================================================

# Policies for newer CMake versions
if (POLICY CMP0126)
  cmake_policy(SET CMP0126 NEW)
endif()

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Release/package semantic version. The umbrella identity below may carry
# additional Git metadata for development builds.
set(_VIX_VERSION_FALLBACK "2.9.0")

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

# PROJECT_VERSION is the stable numeric package version, independent of the
# Git-derived umbrella identity used by development builds.
set(_VIX_PROJECT_VERSION "${_VIX_VERSION_FALLBACK}")

project(vix VERSION ${_VIX_PROJECT_VERSION} LANGUAGES C CXX)

if (WIN32)
  add_compile_definitions(
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
endif()

# Human-friendly version string (can include -N-gHASH[-dirty])
if(_VIX_GIT_DESCRIBE STREQUAL "")
  set(VIX_UMBRELLA_VERSION "v${PROJECT_VERSION}")
else()
  set(VIX_UMBRELLA_VERSION "${_VIX_GIT_DESCRIBE}")
endif()
set(VIX_UMBRELLA_VERSION "${VIX_UMBRELLA_VERSION}" CACHE STRING "Umbrella version" FORCE)

set(VIX_UMBRELLA_BUILD ON CACHE BOOL "Building Vix from umbrella" FORCE)

# Preserve CMake's standard package-root behavior for dependencies.
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

function(vix_enable_sanitizers target)
  if (NOT VIX_ENABLE_SANITIZERS)
    return()
  endif()

  if (NOT TARGET "${target}")
    return()
  endif()

  get_target_property(_target_type "${target}" TYPE)
  if (NOT _target_type)
    return()
  endif()

  if (_target_type STREQUAL "INTERFACE_LIBRARY")
    message(STATUS "Sanitizers: skipping interface target '${target}'")
    return()
  endif()

  if (MSVC)
    message(FATAL_ERROR "Sanitizers are not supported on MSVC. Use Clang or GCC.")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE
      -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

function(vix_enable_sanitizers_first)
  foreach(_candidate IN LISTS ARGN)
    if (TARGET "${_candidate}")
      get_target_property(_aliased "${_candidate}" ALIASED_TARGET)
      if (_aliased)
        vix_enable_sanitizers("${_aliased}")
      else()
        vix_enable_sanitizers("${_candidate}")
      endif()
      return()
    endif()
  endforeach()
endfunction()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
if (APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT WIN32 AND CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    BYPRODUCTS "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Copy compile_commands.json to project root"
    VERBATIM
  )
elseif (WIN32)
  message(STATUS "compile_commands.json copy disabled on Windows.")
endif()

option(VIX_ENABLE_WARNINGS         "Enable strict warnings-as-errors for canonical Vix targets" ON)
option(VIX_ENABLE_INSTALL          "Enable install/export rules for packaging"                    ON)
option(VIX_ENABLE_LTO              "Enable IPO/LTO (Release only)"                                OFF)
option(VIX_MSVC_STATIC_RUNTIME     "Link MSVC runtime statically (/MT)"                           OFF)
option(VIX_ENABLE_CLANG_TIDY       "Enable clang-tidy if available"                               OFF)
option(VIX_ENABLE_CPPCHECK         "Enable cppcheck if available"                                 OFF)
option(VIX_ENABLE_COVERAGE         "Enable coverage flags (Debug only)"                           OFF)
option(VIX_ENABLE_SANITIZERS       "Enable ASan+UBSan (GCC/Clang only)"                           OFF)
option(VIX_BUILD_EXAMPLES          "Build umbrella examples in ./examples"                        ON)
option(VIX_BUILD_TESTS             "Build Vix SDK package and domain tests"                       OFF)
option(VIX_BUILD_NETWORK           "Build the Network domain when its provider is available"      ON)
option(VIX_BUILD_TLS_I0B_TESTS      "Build the private Linux TLS-I0-B feasibility tests"          OFF)
option(VIX_BUILD_TLS_EXPERIMENTAL   "Build the uninstalled experimental TLS Domain"             OFF)

function(vix_apply_canonical_warnings target)
  if (NOT VIX_ENABLE_WARNINGS OR NOT TARGET "${target}")
    return()
  endif()

  get_target_property(_vix_target_type "${target}" TYPE)
  if (_vix_target_type STREQUAL "INTERFACE_LIBRARY" OR _vix_target_type STREQUAL "UTILITY")
    return()
  endif()

  if (MSVC)
    target_compile_options("${target}" PRIVATE /W4 /WX /permissive-)
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options("${target}" PRIVATE
      -Wall -Wextra -Wshadow -Wconversion -Wformat=2 -Wpedantic -Werror)
  endif()
endfunction()

function(vix_apply_canonical_warnings_recursively directory)
  get_property(_vix_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_vix_target IN LISTS _vix_targets)
    if (_vix_target MATCHES "^vix_(standard|files|process|network|concurrency|tls_i0b|tls_experimental)(_|$)")
      vix_apply_canonical_warnings("${_vix_target}")
    endif()
  endforeach()

  get_property(_vix_subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(_vix_subdirectory IN LISTS _vix_subdirectories)
    vix_apply_canonical_warnings_recursively("${_vix_subdirectory}")
  endforeach()
endfunction()

if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(VIX_WASMTIME_EXECUTABLE "" CACHE FILEPATH "Pinned Wasmtime 49.0.3 executable for WASI domain tests")
  if (VIX_WASMTIME_EXECUTABLE STREQUAL "" AND DEFINED WASMTIME_EXECUTABLE)
    set(VIX_WASMTIME_EXECUTABLE "${WASMTIME_EXECUTABLE}")
  endif()
  if (NOT VIX_WASMTIME_EXECUTABLE STREQUAL "")
    set(WASMTIME_EXECUTABLE "${VIX_WASMTIME_EXECUTABLE}")
  endif()
endif()
if (VIX_BUILD_TESTS)
  enable_testing()
endif()

# The root Vix distribution has one public SDK identity. Historical modules
# remain available in their own repositories/standalone builds, but are not
# aggregated into this package.
add_subdirectory(standard standard_build)
add_subdirectory(domains/concurrency concurrency_build)
add_subdirectory(domains/files files_build)
add_subdirectory(domains/process canonical_process_build)
if (VIX_BUILD_NETWORK)
  add_subdirectory(domains/network network_build)
  set(VIX_HAS_NETWORK TRUE)
else()
  set(VIX_HAS_NETWORK FALSE)
endif()

# TLS is explicitly experimental and is never installed/exported. Its current
# lease and runtime qualification is Linux-only; ordinary Vix configurations
# do not discover or link OpenSSL.
if (VIX_BUILD_TLS_EXPERIMENTAL OR VIX_BUILD_TLS_I0B_TESTS)
  if (NOT VIX_HAS_NETWORK OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Experimental TLS currently requires the Linux Network provider.")
  endif()
  find_package(OpenSSL 3 REQUIRED COMPONENTS SSL Crypto)
  add_library(vix_tls_experimental STATIC
    domains/tls/src/OpenSslDriver.cpp
    domains/tls/src/PrivateChannel.cpp
    domains/tls/src/TlsConfig.cpp
    domains/tls/src/TlsChannel.cpp)
  add_library(vix::tls_experimental ALIAS vix_tls_experimental)
  target_compile_features(vix_tls_experimental PUBLIC cxx_std_20)
  target_include_directories(vix_tls_experimental
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/domains/tls/include>
    PRIVATE
      ${CMAKE_SOURCE_DIR}/domains/tls/src
      ${CMAKE_SOURCE_DIR}/domains/network/src)
  target_link_libraries(vix_tls_experimental
    PUBLIC vix::network vix::standard
    PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  if (VIX_BUILD_TLS_I0B_TESTS)
    if (NOT VIX_BUILD_TESTS)
      message(FATAL_ERROR "VIX_BUILD_TLS_I0B_TESTS requires VIX_BUILD_TESTS.")
    endif()
    add_subdirectory(domains/tls/tests tls_i0b_tests)
  endif()
endif()

# Apply diagnostics only to the canonical Vix domain targets and their owned
# tests. Third-party targets and exported consumer usage requirements are not
# affected.
vix_apply_canonical_warnings_recursively("${CMAKE_SOURCE_DIR}")

set(_vix_canonical_targets vix_standard_check vix_concurrency vix_files vix_process)
if (VIX_HAS_NETWORK)
  list(APPEND _vix_canonical_targets vix_network)
endif()
add_custom_target(vix_canonical_architecture DEPENDS ${_vix_canonical_targets})

# Native Process, Network, and Concurrency providers use Threads. Their WASI
# contracts are respectively unsupported or caller-driven/cooperative, so the
# complete WASI SDK stays threadless.
if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(VIX_PACKAGE_NEEDS_THREADS FALSE)
else()
  set(VIX_PACKAGE_NEEDS_THREADS TRUE)
endif()

set(VIX_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Vix")
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VixCanonicalConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
  INSTALL_DESTINATION "${VIX_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(EXPORT VixTargets
  NAMESPACE vix::
  DESTINATION "${VIX_INSTALL_CMAKEDIR}"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/VixConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/VixConfigVersion.cmake"
  DESTINATION "${VIX_INSTALL_CMAKEDIR}"
)
if (VIX_BUILD_TESTS)
  set(_vix_package_toolchain_file "${CMAKE_TOOLCHAIN_FILE}")
  if (NOT _vix_package_toolchain_file STREQUAL "")
    get_filename_component(_vix_package_toolchain_file "${_vix_package_toolchain_file}" REALPATH
      BASE_DIR "${CMAKE_SOURCE_DIR}")
  endif()
  set(_vix_package_sanitizer_link_flags "")
  if (CMAKE_EXE_LINKER_FLAGS MATCHES "-fsanitize")
    set(_vix_package_sanitizer_link_flags "${CMAKE_EXE_LINKER_FLAGS}")
  endif()
  add_test(
    NAME vix_sdk_package_consumer_test
    COMMAND "${CMAKE_COMMAND}"
      "-DVIX_BINARY_DIR=${CMAKE_BINARY_DIR}"
      "-DVIX_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
      "-DVIX_EXPECT_NETWORK=${VIX_HAS_NETWORK}"
      "-DVIX_TARGET_SYSTEM=${CMAKE_SYSTEM_NAME}"
      "-DVIX_CONFIGURATION=$<CONFIG>"
      "-DVIX_GENERATOR=${CMAKE_GENERATOR}"
      "-DVIX_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DVIX_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
      "-DVIX_WASMTIME_EXECUTABLE=${VIX_WASMTIME_EXECUTABLE}"
      "-DVIX_WASI_SDK_ROOT=${VIX_WASI_SDK_ROOT}"
      "-DVIX_TOOLCHAIN_FILE=${_vix_package_toolchain_file}"
      "-DVIX_SANITIZER_LINK_FLAGS=${_vix_package_sanitizer_link_flags}"
      "-DVIX_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VixSdkPackageTest.cmake"
  )
endif()

if (VIX_HAS_NETWORK)
  message(STATUS "Vix SDK: Standard -> Concurrency, Files -> Process -> Network")
else()
  message(STATUS "Vix SDK: Standard -> Concurrency, Files -> Process (Network unavailable/not requested)")
endif()
