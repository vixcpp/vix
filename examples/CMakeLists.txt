cmake_minimum_required(VERSION 3.20)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_example_core_root LANGUAGES CXX)
  find_package(Vix CONFIG REQUIRED)
endif()

set(_vix_core_root_examples
  01_run_blocking
  02_listen_background
  03_listen_port_dynamic
  hello_routes
  main
  vix_routes_showcase
)

foreach (_example IN LISTS _vix_core_root_examples)
  add_executable("core_root_${_example}" "${_example}.cpp")
  target_compile_features("core_root_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("core_root_${_example}" PRIVATE vix::core)
endforeach()
