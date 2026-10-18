cmake_minimum_required(VERSION 3.20)
project(vix_sdk_consumer LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

foreach(target
    vix::vix_standard
    vix::vix_concurrency
    vix::vix_files
    vix::vix_process)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "missing imported target: ${target}")
  endif()
endforeach()
if (NOT TARGET vix::vix_network)
  message(FATAL_ERROR "this consumer requires the installed Network component")
endif()

add_executable(vix_sdk_consumer main.cpp)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vix_sdk_consumer_path-$<CONFIG>.txt"
  CONTENT "$<TARGET_FILE:vix_sdk_consumer>\n")
target_link_libraries(vix_sdk_consumer PRIVATE
  vix::vix_standard
  vix::vix_concurrency
  vix::vix_files
  vix::vix_process
  vix::vix_network)
