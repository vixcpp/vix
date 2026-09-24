cmake_minimum_required(VERSION 3.20)

project(vix_example_requests LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

file(GLOB _vix_request_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach (_source IN LISTS _vix_request_sources)
  get_filename_component(_name "${_source}" NAME_WE)
  add_executable("requests_${_name}" "${_source}")
  target_compile_features("requests_${_name}" PRIVATE cxx_std_20)
  target_link_libraries("requests_${_name}" PRIVATE vix::requests)
endforeach()
