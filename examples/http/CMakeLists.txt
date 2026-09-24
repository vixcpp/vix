cmake_minimum_required(VERSION 3.20)

project(vix_example_http LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

file(GLOB _vix_http_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach (_source IN LISTS _vix_http_sources)
  get_filename_component(_name "${_source}" NAME_WE)
  add_executable("http_${_name}" "${_source}")
  target_compile_features("http_${_name}" PRIVATE cxx_std_20)
  target_link_libraries("http_${_name}" PRIVATE vix::core)
endforeach()
