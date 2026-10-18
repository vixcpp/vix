cmake_minimum_required(VERSION 3.20)
project(vix_sdk_files_process_consumer LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED COMPONENTS Concurrency Files Process)

foreach(target vix::vix_standard vix::vix_concurrency vix::vix_files vix::vix_process)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "missing imported target: ${target}")
  endif()
endforeach()
if (TARGET vix::vix_network)
  message(FATAL_ERROR "Network-disabled package unexpectedly exports vix::vix_network")
endif()

add_executable(vix_sdk_files_process_consumer main.cpp)
target_link_libraries(vix_sdk_files_process_consumer PRIVATE
  vix::vix_standard vix::vix_concurrency vix::vix_files vix::vix_process)
