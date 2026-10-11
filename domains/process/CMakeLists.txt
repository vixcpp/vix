cmake_minimum_required(VERSION 3.23)
include(GNUInstallDirs)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_process LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  option(VIX_BUILD_TESTS "Build focused Vix Process tests" OFF)
  add_subdirectory(../../standard standard_build)
  add_subdirectory(../files files_build)
endif()

if (NOT UNIX)
  message(FATAL_ERROR "The initial vix_process provider is POSIX-only.")
endif()

find_package(Threads REQUIRED)

add_library(vix_process src/ProcessPosix.cpp)
add_library(vix::process ALIAS vix_process)
target_compile_features(vix_process PUBLIC cxx_std_20)
target_include_directories(vix_process PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(vix_process PUBLIC vix::standard vix::files PRIVATE Threads::Threads)

install(TARGETS vix_process EXPORT VixTargets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING PATTERN "*.hpp")

if (VIX_BUILD_TESTS)
  if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
  endif()
  add_subdirectory(tests)
endif()
