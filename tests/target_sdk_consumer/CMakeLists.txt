cmake_minimum_required(VERSION 3.20)
project(vix_target_sdk_consumer LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

foreach(target vix::vix_standard vix::vix_concurrency vix::vix_files vix::vix_process vix::vix_network)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "Missing installed Vix target: ${target}")
  endif()
endforeach()

add_executable(vix_target_sdk_consumer main.cpp)
target_link_libraries(vix_target_sdk_consumer PRIVATE
  vix::vix_standard vix::vix_concurrency vix::vix_files vix::vix_process vix::vix_network)
