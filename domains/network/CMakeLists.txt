cmake_minimum_required(VERSION 3.23)
include(GNUInstallDirs)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_network LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  # Standalone builds expose only the focused Network test entry points.
  option(VIX_BUILD_TESTS "Build focused Vix Network phase tests" ON)
  add_subdirectory(../../standard standard_build)
endif()
# Select native polling and wakeup implementations explicitly. Android uses
# the Linux-family provider through NDK/Bionic; Darwin uses kqueue, Windows
# uses Winsock/WSAPoll, and WASI Preview 2 uses cooperative wasi-libc poll.
if (ANDROID)
  # The Linux-family provider uses epoll/eventfd, accept4, and atomic socket
  # flags available from Android API 21 onward. Keep supported NDK ABIs
  # explicit so an untested ABI is not accidentally advertised.
  if (NOT CMAKE_ANDROID_ARCH_ABI MATCHES "^(arm64-v8a|x86_64)$")
    message(FATAL_ERROR
      "Vix Network Android support is limited to arm64-v8a and x86_64; got '${CMAKE_ANDROID_ARCH_ABI}'.")
  endif()
  set(_vix_network_android_api "")
  if (DEFINED ANDROID_PLATFORM_LEVEL)
    set(_vix_network_android_api "${ANDROID_PLATFORM_LEVEL}")
  elseif (ANDROID_PLATFORM MATCHES "android-([0-9]+)$")
    set(_vix_network_android_api "${CMAKE_MATCH_1}")
  elseif (DEFINED CMAKE_ANDROID_API)
    set(_vix_network_android_api "${CMAKE_ANDROID_API}")
  elseif (CMAKE_SYSTEM_VERSION MATCHES "^([0-9]+)$")
    set(_vix_network_android_api "${CMAKE_MATCH_1}")
  elseif (CMAKE_SYSTEM_VERSION MATCHES "android-([0-9]+)$")
    set(_vix_network_android_api "${CMAKE_MATCH_1}")
  endif()
  if (_vix_network_android_api STREQUAL "" OR _vix_network_android_api LESS 21)
    message(FATAL_ERROR
      "Vix Network Android requires NDK platform/API level 21 or newer (epoll_create1, eventfd, accept4).")
  endif()
  set(VIX_NETWORK_PLATFORM_BACKEND "android")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(VIX_NETWORK_PLATFORM_BACKEND "linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(VIX_NETWORK_PLATFORM_BACKEND "macos")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(VIX_NETWORK_PLATFORM_BACKEND "windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "WASI" AND CMAKE_CXX_COMPILER_TARGET STREQUAL "wasm32-wasip2")
  set(VIX_NETWORK_PLATFORM_BACKEND "wasi-p2")
else()
  if (CMAKE_SYSTEM_NAME MATCHES "^(WASI|Emscripten)$")
    message(FATAL_ERROR
      "Vix Network supports only the cooperative wasi-libc Preview 2 provider for wasm32-wasip2; Preview 1 and browser WebAssembly do not provide this raw TCP contract.")
  endif()
  message(FATAL_ERROR
    "Vix Network has no provider for ${CMAKE_SYSTEM_NAME}; implemented providers: Linux, Android, macOS, and Windows.")
endif()
if (NOT VIX_NETWORK_PLATFORM_BACKEND STREQUAL "wasi-p2")
  find_package(Threads REQUIRED)
endif()
set(VIX_NETWORK_PLATFORM_SOURCES)
if (VIX_NETWORK_PLATFORM_BACKEND STREQUAL "linux" OR VIX_NETWORK_PLATFORM_BACKEND STREQUAL "android")
  list(APPEND VIX_NETWORK_PLATFORM_SOURCES src/LinuxReactor.cpp src/PosixSocket.cpp)
elseif (VIX_NETWORK_PLATFORM_BACKEND STREQUAL "macos")
  list(APPEND VIX_NETWORK_PLATFORM_SOURCES src/MacReactor.cpp src/PosixSocket.cpp)
elseif (VIX_NETWORK_PLATFORM_BACKEND STREQUAL "windows")
  list(APPEND VIX_NETWORK_PLATFORM_SOURCES src/WindowsReactor.cpp src/WindowsSocket.cpp)
elseif (VIX_NETWORK_PLATFORM_BACKEND STREQUAL "wasi-p2")
  list(APPEND VIX_NETWORK_PLATFORM_SOURCES src/WasiReactor.cpp src/PosixSocket.cpp)
else()
  message(FATAL_ERROR "No Vix Network provider selected for ${CMAKE_SYSTEM_NAME}.")
endif()
add_library(vix_network STATIC src/Engine.cpp src/Network.cpp src/Address.cpp ${VIX_NETWORK_PLATFORM_SOURCES})
add_library(vix::network ALIAS vix_network)
target_compile_features(vix_network PUBLIC cxx_std_20)
target_include_directories(vix_network
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../standard/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE src)
# Public Network headers expose vix::Result, Completion, Deadline and
# CancellationToken, so consumers of the exported target need Standard too.
target_link_libraries(vix_network PUBLIC vix::standard)
if (NOT VIX_NETWORK_PLATFORM_BACKEND STREQUAL "wasi-p2")
  target_link_libraries(vix_network PRIVATE Threads::Threads)
endif()
if (VIX_NETWORK_PLATFORM_BACKEND STREQUAL "windows")
  # WSAPoll and address conversion APIs require Windows Vista or newer. The
  # umbrella project supplies its own newer baseline; standalone builds need it.
  if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_compile_definitions(vix_network PRIVATE _WIN32_WINNT=0x0600)
  endif()
  target_link_libraries(vix_network PRIVATE ws2_32)
endif()
install(TARGETS vix_network EXPORT VixTargets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.hpp")
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  install(DIRECTORY ../../standard/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.hpp")
endif()
if (VIX_BUILD_TESTS)
  target_compile_definitions(vix_network PRIVATE VIX_NETWORK_TEST_CAPTURE=1)
  if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
  endif()
  add_subdirectory(tests)
endif()
