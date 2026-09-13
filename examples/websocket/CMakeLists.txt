cmake_minimum_required(VERSION 3.20)

project(vix_example_websocket LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

file(GLOB_RECURSE _vix_websocket_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach (_source IN LISTS _vix_websocket_sources)
  file(RELATIVE_PATH _relative "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
  string(REPLACE "/" "_" _name "${_relative}")
  string(REPLACE "\\" "_" _name "${_name}")
  string(REGEX REPLACE "\\.cpp$" "" _name "${_name}")
  add_executable("websocket_${_name}" "${_source}")
  target_compile_features("websocket_${_name}" PRIVATE cxx_std_20)
  target_link_libraries("websocket_${_name}" PRIVATE vix::websocket)
endforeach()
