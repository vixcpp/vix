cmake_minimum_required(VERSION 3.20)

project(vix_installed_sdk_consumer_contract LANGUAGES CXX)

if (NOT DEFINED VIX_SDK_PREFIX)
  message(FATAL_ERROR "VIX_SDK_PREFIX must name the isolated installed Vix SDK.")
endif()

set(VIX_CONSUMER_PROFILE "" CACHE STRING
  "Installed Vix SDK profile to certify (default/web/data/desktop/p2p/game/agent/all)")
set_property(CACHE VIX_CONSUMER_PROFILE PROPERTY STRINGS
  default web data desktop p2p game agent all)
option(VIX_CONSUMER_GAME_BACKENDS
  "Compile/link the installed SDL and OpenGL vix::game backend contract"
  OFF)
option(VIX_CONSUMER_MYSQL_BACKEND
  "Run the installed no-server MySQL vix::db backend contract"
  OFF)

set(_vix_supported_profiles default web data desktop p2p game agent all)
if (NOT VIX_CONSUMER_PROFILE IN_LIST _vix_supported_profiles)
  message(FATAL_ERROR
    "VIX_CONSUMER_PROFILE must be one of: ${_vix_supported_profiles}")
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

set(_vix_base_modules
  error path fs io env os utils log core json time conversion async process
  threadpool template ui reply note net sync engine)
set(_vix_profile_modules ${_vix_base_modules})

if (VIX_CONSUMER_PROFILE STREQUAL "web")
  list(APPEND _vix_profile_modules
    websocket realtime middleware validation webrpc crypto requests cache)
elseif (VIX_CONSUMER_PROFILE STREQUAL "data")
  list(APPEND _vix_profile_modules db orm kv cache)
elseif (VIX_CONSUMER_PROFILE STREQUAL "p2p")
  list(APPEND _vix_profile_modules p2p p2p_http crypto cache)
elseif (VIX_CONSUMER_PROFILE STREQUAL "game")
  list(APPEND _vix_profile_modules game)
elseif (VIX_CONSUMER_PROFILE STREQUAL "agent")
  list(APPEND _vix_profile_modules agent crypto cache)
elseif (VIX_CONSUMER_PROFILE STREQUAL "all")
  list(APPEND _vix_profile_modules
    websocket realtime middleware validation webrpc crypto requests db orm kv cache
    p2p p2p_http game agent tests)
endif()
list(REMOVE_DUPLICATES _vix_profile_modules)

set(_vix_profile_targets vix::vix)
foreach (_module IN LISTS _vix_profile_modules)
  if (_module STREQUAL "agent")
    list(APPEND _vix_profile_targets vix::ai_agent)
  else()
    list(APPEND _vix_profile_targets "vix::${_module}")
  endif()
endforeach()
list(SORT _vix_profile_targets)

set(_vix_profile_specific_targets
  vix::websocket vix::realtime vix::middleware vix::validation vix::webrpc
  vix::crypto vix::requests vix::db vix::orm vix::kv vix::cache vix::p2p
  vix::p2p_http vix::game vix::ai_agent vix::tests)
set(_vix_forbidden_targets ${_vix_profile_specific_targets})
foreach (_target IN LISTS _vix_profile_targets)
  list(REMOVE_ITEM _vix_forbidden_targets "${_target}")
endforeach()

foreach (_vix_public_target IN LISTS _vix_profile_targets)
  if (NOT TARGET "${_vix_public_target}")
    message(FATAL_ERROR
      "Installed Vix ${VIX_CONSUMER_PROFILE} SDK is missing public target ${_vix_public_target}")
  endif()
endforeach()

foreach (_vix_forbidden_target IN LISTS _vix_forbidden_targets)
  if (TARGET "${_vix_forbidden_target}")
    message(FATAL_ERROR
      "Installed Vix ${VIX_CONSUMER_PROFILE} SDK unexpectedly exports profile-specific target ${_vix_forbidden_target}")
  endif()
endforeach()

function(vix_header_owner relative_header output)
  if (relative_header STREQUAL "vix/all.hpp")
    set(${output} all PARENT_SCOPE)
    return()
  endif()

  if (relative_header MATCHES "^vix/(config|executor|server|runtime|openapi|session|http|mw|router|app|timers|view)/")
    set(${output} core PARENT_SCOPE)
    return()
  endif()

  if (relative_header MATCHES "^vix/(io|meta|console|format|inspect|print)/" OR
      relative_header MATCHES "^vix/(print|format|inspect|input|console)\\.(hpp|h)$")
    set(${output} io PARENT_SCOPE)
    return()
  endif()

  if (relative_header MATCHES "^vix/ai/")
    set(${output} agent PARENT_SCOPE)
    return()
  endif()

  if (relative_header STREQUAL "vix/Version.hpp")
    set(${output} core PARENT_SCOPE)
    return()
  endif()

  if (relative_header MATCHES "^vix/([^/]+)\\.(hpp|h)$")
    set(${output} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    return()
  endif()

  if (relative_header MATCHES "^vix/([^/]+)/")
    set(${output} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    return()
  endif()

  set(${output} "" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE _vix_installed_headers LIST_DIRECTORIES FALSE
  "${VIX_SDK_PREFIX}/include/vix/*.hpp"
  "${VIX_SDK_PREFIX}/include/vix/*.h"
)
list(SORT _vix_installed_headers)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/headers")
set(_vix_header_sources "")
set(_vix_header_modules "")
foreach (_header IN LISTS _vix_installed_headers)
  file(RELATIVE_PATH _relative_header
    "${VIX_SDK_PREFIX}/include"
    "${_header}"
  )
  string(REPLACE "\\" "/" _relative_header "${_relative_header}")
  if (_relative_header MATCHES "^vix/third_party/")
    continue()
  endif()

  vix_header_owner("${_relative_header}" _header_module)
  if (NOT _header_module)
    message(FATAL_ERROR
      "Cannot assign installed first-party header to a Vix module: ${_relative_header}")
  endif()

  if (_header_module STREQUAL "all")
    if (NOT VIX_CONSUMER_PROFILE STREQUAL "all")
      message(FATAL_ERROR
        "Installed Vix ${VIX_CONSUMER_PROFILE} SDK unexpectedly contains ${_relative_header}")
    endif()
  elseif (NOT _header_module IN_LIST _vix_profile_modules)
    message(FATAL_ERROR
      "Installed Vix ${VIX_CONSUMER_PROFILE} SDK leaks header from disabled module ${_header_module}: ${_relative_header}")
  endif()

  string(REPLACE "/" "__" _source_name "${_relative_header}")
  string(REPLACE "." "_" _source_name "${_source_name}")
  set(_source "${CMAKE_CURRENT_BINARY_DIR}/headers/${_source_name}.cpp")
  file(WRITE "${_source}"
    "#include <${_relative_header}>\n\nint main() { return 0; }\n"
  )
  list(APPEND _vix_header_sources "${_source}")
  list(APPEND _vix_header_modules "${_header_module}")
  list(APPEND _vix_header_sources_${_header_module} "${_source}")
endforeach()

if (NOT _vix_header_sources)
  message(FATAL_ERROR "No installed first-party Vix headers were discovered.")
endif()

# Every generated source remains a separate translation unit. Group them by
# owning module and consume each module's installed public target, so headers
# receive precisely that target's PUBLIC/INTERFACE usage requirements.
list(REMOVE_DUPLICATES _vix_header_modules)
foreach (_header_module IN LISTS _vix_header_modules)
  if (_header_module STREQUAL "all")
    set(_vix_header_owner_target vix::vix)
  elseif (_header_module STREQUAL "agent")
    set(_vix_header_owner_target vix::ai_agent)
  else()
    set(_vix_header_owner_target "vix::${_header_module}")
  endif()

  if (NOT TARGET "${_vix_header_owner_target}")
    message(FATAL_ERROR
      "Installed Vix header owner target is unavailable: ${_vix_header_owner_target}")
  endif()

  string(REPLACE "::" "__" _vix_header_target_id "${_vix_header_owner_target}")
  add_library("vix_installed_${_vix_header_target_id}_header_contract" OBJECT
    ${_vix_header_sources_${_header_module}}
  )
  target_compile_features(
    "vix_installed_${_vix_header_target_id}_header_contract"
    PRIVATE cxx_std_20
  )
  target_link_libraries(
    "vix_installed_${_vix_header_target_id}_header_contract"
    PRIVATE "${_vix_header_owner_target}"
  )
endforeach()

foreach (_vix_public_target IN LISTS _vix_profile_targets)
  string(REPLACE "::" "__" _target_id "${_vix_public_target}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/targets")
  set(_source "${CMAKE_CURRENT_BINARY_DIR}/targets/${_target_id}.cpp")
  file(WRITE "${_source}" "int main() { return 0; }\n")

  add_executable("${_target_id}_consumer" "${_source}")
  target_compile_features("${_target_id}_consumer" PRIVATE cxx_std_20)
  target_link_libraries("${_target_id}_consumer" PRIVATE "${_vix_public_target}")
endforeach()

if (VIX_CONSUMER_GAME_BACKENDS)
  if (NOT VIX_CONSUMER_PROFILE STREQUAL "game" AND
      NOT VIX_CONSUMER_PROFILE STREQUAL "all")
    message(FATAL_ERROR
      "VIX_CONSUMER_GAME_BACKENDS is valid only for game or all profiles")
  endif()

  set(_vix_game_backend_source
    "${CMAKE_CURRENT_BINARY_DIR}/targets/game_sdl_opengl_consumer.cpp")
  file(WRITE "${_vix_game_backend_source}" [=[
#include <memory>

#include <vix/game/backends/sdl/SDLOpenGLRenderer.hpp>
#include <vix/game/backends/sdl/SDLWindow.hpp>

int main()
{
  auto window = std::make_unique<vix::game::sdl::SDLWindow>();
  auto renderer = std::make_unique<vix::game::sdl::SDLOpenGLRenderer>();
  return (window && renderer) ? 0 : 1;
}
]=])

  add_executable(vix_game_sdl_opengl_consumer "${_vix_game_backend_source}")
  target_compile_features(vix_game_sdl_opengl_consumer PRIVATE cxx_std_20)
  target_link_libraries(vix_game_sdl_opengl_consumer PRIVATE vix::game)
endif()

if (VIX_CONSUMER_MYSQL_BACKEND)
  if (NOT VIX_CONSUMER_PROFILE STREQUAL "all")
    message(FATAL_ERROR
      "VIX_CONSUMER_MYSQL_BACKEND is valid only for the all profile")
  endif()

  enable_testing()

  set(_vix_mysql_backend_source
    "${CMAKE_CURRENT_BINARY_DIR}/targets/mysql_backend_consumer.cpp")
  file(WRITE "${_vix_mysql_backend_source}" [=[
#include <vix/db/Database.hpp>
#include <vix/db/pool/ConnectionPool.hpp>

int main()
{
  try
  {
    vix::db::PoolConfig pool;
    pool.min = 0;
    pool.max = 1;

    auto db = vix::db::Database::mysql(
        "tcp://127.0.0.1:3306", "", "", "", pool);
    return db.engine() == vix::db::Engine::MySQL ? 0 : 1;
  }
  catch (...)
  {
    return 1;
  }
}
]=])

  add_executable(vix_mysql_backend_consumer "${_vix_mysql_backend_source}")
  target_compile_features(vix_mysql_backend_consumer PRIVATE cxx_std_20)
  target_link_libraries(vix_mysql_backend_consumer PRIVATE vix::db)
  add_test(NAME vix_mysql_backend_consumer COMMAND vix_mysql_backend_consumer)
endif()
