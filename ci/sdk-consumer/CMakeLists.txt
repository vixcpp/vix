cmake_minimum_required(VERSION 3.20)

project(vix_installed_sdk_consumer_contract LANGUAGES CXX)

if (NOT DEFINED VIX_SDK_PREFIX)
  message(FATAL_ERROR "VIX_SDK_PREFIX must name the isolated installed Vix SDK.")
endif()

get_filename_component(VIX_SDK_PREFIX "${VIX_SDK_PREFIX}" ABSOLUTE)
if (NOT IS_DIRECTORY "${VIX_SDK_PREFIX}/include")
  message(FATAL_ERROR "VIX_SDK_PREFIX does not contain include/: ${VIX_SDK_PREFIX}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Resolve only the installed prefix supplied by the workflow. This prevents a
# build-tree, source-tree, user registry, or ambient CMAKE_PREFIX_PATH package
# from satisfying the contract accidentally.
unset(Vix_DIR CACHE)
unset(vix_DIR CACHE)
unset(VIX_ROOT CACHE)
unset(ENV{VIX_ROOT})
unset(ENV{Vix_DIR})
unset(ENV{vix_DIR})
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
find_package(Vix CONFIG REQUIRED
  PATHS "${VIX_SDK_PREFIX}"
  NO_DEFAULT_PATH
)

file(GLOB_RECURSE _vix_installed_headers LIST_DIRECTORIES FALSE
  "${VIX_SDK_PREFIX}/include/vix/*.hpp"
  "${VIX_SDK_PREFIX}/include/vix/*.h"
)
list(SORT _vix_installed_headers)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/headers")
set(_vix_header_sources "")
foreach (_header IN LISTS _vix_installed_headers)
  file(RELATIVE_PATH _relative_header
    "${VIX_SDK_PREFIX}/include"
    "${_header}"
  )
  string(REPLACE "\\" "/" _relative_header "${_relative_header}")
  if (_relative_header MATCHES "^vix/third_party/")
    continue()
  endif()

  string(REPLACE "/" "__" _source_name "${_relative_header}")
  string(REPLACE "." "_" _source_name "${_source_name}")
  set(_source "${CMAKE_CURRENT_BINARY_DIR}/headers/${_source_name}.cpp")
  file(WRITE "${_source}"
    "#include <${_relative_header}>\n\nint main() { return 0; }\n"
  )
  list(APPEND _vix_header_sources "${_source}")
endforeach()

if (NOT _vix_header_sources)
  message(FATAL_ERROR "No installed first-party Vix headers were discovered.")
endif()

# Every generated source remains a separate translation unit. The target uses
# only the installed SDK include directory, never a source-tree include path.
add_library(vix_installed_header_contract OBJECT ${_vix_header_sources})
target_compile_features(vix_installed_header_contract PRIVATE cxx_std_20)
target_include_directories(vix_installed_header_contract PRIVATE
  "${VIX_SDK_PREFIX}/include"
)

set(_vix_public_targets
  vix::ai_agent
  vix::async
  vix::cache
  vix::conversion
  vix::core
  vix::crypto
  vix::db
  vix::engine
  vix::env
  vix::error
  vix::fs
  vix::game
  vix::io
  vix::json
  vix::kv
  vix::log
  vix::middleware
  vix::net
  vix::note
  vix::orm
  vix::os
  vix::p2p
  vix::p2p_http
  vix::path
  vix::process
  vix::realtime
  vix::reply
  vix::requests
  vix::sync
  vix::template
  vix::tests
  vix::threadpool
  vix::time
  vix::ui
  vix::utils
  vix::validation
  vix::webrpc
  vix::websocket
  vix::vix
)

foreach (_vix_public_target IN LISTS _vix_public_targets)
  if (NOT TARGET "${_vix_public_target}")
    message(FATAL_ERROR "Installed Vix package is missing public target ${_vix_public_target}")
  endif()

  string(REPLACE "::" "__" _target_id "${_vix_public_target}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/targets")
  set(_source "${CMAKE_CURRENT_BINARY_DIR}/targets/${_target_id}.cpp")
  file(WRITE "${_source}" "int main() { return 0; }\n")

  add_executable("${_target_id}_consumer" "${_source}")
  target_compile_features("${_target_id}_consumer" PRIVATE cxx_std_20)
  target_link_libraries("${_target_id}_consumer" PRIVATE "${_vix_public_target}")
endforeach()
