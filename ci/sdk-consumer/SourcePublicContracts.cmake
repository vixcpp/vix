cmake_minimum_required(VERSION 3.20)

if (NOT DEFINED VIX_SOURCE_DIR)
  message(FATAL_ERROR "VIX_SOURCE_DIR is required.")
endif()

get_filename_component(VIX_SOURCE_DIR "${VIX_SOURCE_DIR}" ABSOLUTE)

set(_vix_public_files "")
set(_vix_installed_paths "")

file(GLOB_RECURSE _vix_root_headers LIST_DIRECTORIES FALSE
  "${VIX_SOURCE_DIR}/vix/*.hpp"
  "${VIX_SOURCE_DIR}/vix/*.h"
)

foreach (_header IN LISTS _vix_root_headers)
  file(RELATIVE_PATH _installed_path "${VIX_SOURCE_DIR}" "${_header}")
  string(REPLACE "\\" "/" _installed_path "${_installed_path}")
  if (NOT _installed_path MATCHES "^vix/third_party/")
    list(APPEND _vix_public_files "${_header}")
    list(APPEND _vix_installed_paths "${_installed_path}")
  endif()
endforeach()

file(GLOB _vix_module_include_roots LIST_DIRECTORIES TRUE
  "${VIX_SOURCE_DIR}/modules/*/include/vix"
)

foreach (_include_root IN LISTS _vix_module_include_roots)
  file(GLOB_RECURSE _module_headers LIST_DIRECTORIES FALSE
    "${_include_root}/*.hpp"
    "${_include_root}/*.h"
  )

  foreach (_header IN LISTS _module_headers)
    file(RELATIVE_PATH _relative_header "${_include_root}" "${_header}")
    string(REPLACE "\\" "/" _relative_header "${_relative_header}")
    set(_installed_path "vix/${_relative_header}")
    if (NOT _installed_path MATCHES "^vix/third_party/")
      list(APPEND _vix_public_files "${_header}")
      list(APPEND _vix_installed_paths "${_installed_path}")
    endif()
  endforeach()
endforeach()

list(LENGTH _vix_public_files _vix_public_file_count)
if (_vix_public_file_count EQUAL 0)
  message(FATAL_ERROR "No first-party public Vix headers were found.")
endif()

set(_vix_duplicate_paths "")
set(_vix_duplicate_guards "")
math(EXPR _vix_last_index "${_vix_public_file_count} - 1")

foreach (_index RANGE 0 ${_vix_last_index})
  list(GET _vix_public_files ${_index} _header)
  list(GET _vix_installed_paths ${_index} _installed_path)

  string(SHA256 _path_hash "${_installed_path}")
  set(_path_owners_var "_vix_path_owners_${_path_hash}")
  if (DEFINED ${_path_owners_var})
    list(APPEND _vix_duplicate_paths "${_installed_path}")
  endif()
  list(APPEND ${_path_owners_var} "${_header}")

  file(STRINGS "${_header}" _guard_lines
    REGEX "^[ \t]*#ifndef[ \t]+VIX_[A-Z0-9_]+_HPP"
  )
  if (_guard_lines)
    list(GET _guard_lines 0 _guard_line)
    string(REGEX REPLACE
      "^[ \t]*#ifndef[ \t]+(VIX_[A-Z0-9_]+_HPP).*$"
      "\\1"
      _guard
      "${_guard_line}"
    )

    string(SHA256 _guard_hash "${_guard}")
    set(_guard_owners_var "_vix_guard_owners_${_guard_hash}")
    if (DEFINED ${_guard_owners_var})
      list(APPEND _vix_duplicate_guards "${_guard}")
    endif()
    list(APPEND ${_guard_owners_var} "${_header}")
  endif()
endforeach()

list(REMOVE_DUPLICATES _vix_duplicate_paths)
if (_vix_duplicate_paths)
  message(STATUS "Duplicate installed first-party public paths:")
  foreach (_installed_path IN LISTS _vix_duplicate_paths)
    string(SHA256 _path_hash "${_installed_path}")
    set(_path_owners_var "_vix_path_owners_${_path_hash}")
    message(STATUS "  ${_installed_path}")
    foreach (_owner IN LISTS ${_path_owners_var})
      message(STATUS "    ${_owner}")
    endforeach()
  endforeach()
  message(FATAL_ERROR "First-party public headers have duplicate installed paths.")
endif()

list(REMOVE_DUPLICATES _vix_duplicate_guards)
if (_vix_duplicate_guards)
  message(STATUS "Duplicate first-party public include guards:")
  foreach (_guard IN LISTS _vix_duplicate_guards)
    string(SHA256 _guard_hash "${_guard}")
    set(_guard_owners_var "_vix_guard_owners_${_guard_hash}")
    message(STATUS "  ${_guard}")
    foreach (_owner IN LISTS ${_guard_owners_var})
      message(STATUS "    ${_owner}")
    endforeach()
  endforeach()
  message(FATAL_ERROR "First-party public headers have duplicate include guards.")
endif()

message(STATUS
  "Validated ${_vix_public_file_count} first-party public headers: "
  "installed paths and VIX_* include guards are unique."
)
