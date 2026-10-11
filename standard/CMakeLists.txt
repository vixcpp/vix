cmake_minimum_required(VERSION 3.23)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_standard LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  option(VIX_BUILD_TESTS "Build focused Vix Standard tests" OFF)
endif()

# The Vix Standard is deliberately independent of modules/*.  Its approved
# primitives are header-defined; a verification target is intentionally kept
# separate from this dependency-only target.
add_library(vix_standard INTERFACE)
add_library(vix::standard ALIAS vix_standard)

target_compile_features(vix_standard INTERFACE cxx_std_20)

target_include_directories(vix_standard
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# The umbrella's existing test gate owns whether focused Standard tests are
# configured.  No module test option or module dependency is involved.
if (VIX_BUILD_TESTS)
  if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
  endif()

  add_subdirectory(tests)
else()
  add_custom_target(vix_standard_check
    COMMENT "Vix Standard verification is disabled; configure with VIX_BUILD_TESTS=ON")
endif()
