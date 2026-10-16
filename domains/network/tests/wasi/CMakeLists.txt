cmake_minimum_required(VERSION 3.23)
project(vix_wasi_p2_socket_abi_probe LANGUAGES C)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "WASI" OR NOT CMAKE_C_COMPILER_TARGET STREQUAL "wasm32-wasip2")
  message(FATAL_ERROR "Configure this probe with wasi-sdk 34.0's wasi-sdk-p2.cmake toolchain.")
endif()

add_executable(vix_wasi_p2_socket_contract ../p2_socket_contract.c)
set_target_properties(vix_wasi_p2_socket_contract PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

if (NOT DEFINED WASMTIME_EXECUTABLE OR WASMTIME_EXECUTABLE STREQUAL "" OR
    NOT EXISTS "${WASMTIME_EXECUTABLE}")
  message(FATAL_ERROR "Set WASMTIME_EXECUTABLE to the pinned Wasmtime 49.0.3 CLI; socket tests must not be silently omitted.")
endif()

enable_testing()
add_test(NAME vix_wasi_p2_host_denies_network
  COMMAND "${WASMTIME_EXECUTABLE}" run -W tcp=y
          "$<TARGET_FILE:vix_wasi_p2_socket_contract>" denied)
add_test(NAME vix_wasi_p2_loopback_tcp
  COMMAND "${WASMTIME_EXECUTABLE}" run -W tcp=y -W inherit-network=y
          "$<TARGET_FILE:vix_wasi_p2_socket_contract>" loopback)
