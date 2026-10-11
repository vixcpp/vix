cmake_minimum_required(VERSION 3.23)
include(GNUInstallDirs)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_files LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  option(VIX_BUILD_TESTS "Build focused Vix Files tests" OFF)
  add_subdirectory(../../standard standard_build)
endif()

if (NOT UNIX)
  message(FATAL_ERROR "The initial vix_files provider is POSIX-only; no Windows provider is supplied by this patch.")
endif()

add_library(vix_files
  src/FilesPosix.cpp
)
add_library(vix::files ALIAS vix_files)

target_compile_features(vix_files PUBLIC cxx_std_20)
target_include_directories(vix_files PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(vix_files PUBLIC vix::standard)

install(TARGETS vix_files EXPORT VixTargets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING PATTERN "*.hpp" PATTERN "detail/*" EXCLUDE)

if (VIX_BUILD_TESTS)
  if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
  endif()
  add_subdirectory(tests)
endif()
