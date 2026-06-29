cmake_minimum_required(VERSION 3.20)

project(vix_examples_root LANGUAGES CXX)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/requests/CMakeLists.txt")
  message(STATUS "Examples: enabling Requests examples (examples/requests)")
  add_subdirectory(requests)
endif()
