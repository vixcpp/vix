if (NOT CMAKE_SYSTEM_NAME STREQUAL "WASI" OR
    NOT CMAKE_CXX_COMPILER_TARGET STREQUAL "wasm32-wasip2")
  message(FATAL_ERROR "Configure Files WASI tests with wasi-sdk 34.0 and wasm32-wasip2.")
endif()

if (NOT DEFINED WASMTIME_EXECUTABLE OR WASMTIME_EXECUTABLE STREQUAL "" OR
    NOT EXISTS "${WASMTIME_EXECUTABLE}")
  message(FATAL_ERROR "Set WASMTIME_EXECUTABLE; WASI filesystem capability tests must not be omitted.")
endif()

add_executable(vix_files_wasi_capability_test capability_test.cpp)
target_compile_features(vix_files_wasi_capability_test PRIVATE cxx_std_20)
target_link_libraries(vix_files_wasi_capability_test PRIVATE vix::files)

set(_vix_files_wasi_preopen "${CMAKE_CURRENT_BINARY_DIR}/preopen")
file(MAKE_DIRECTORY "${_vix_files_wasi_preopen}")
add_test(NAME vix_files_wasi_preopen_contract
  COMMAND "${WASMTIME_EXECUTABLE}" run
          --dir "${_vix_files_wasi_preopen}::/workspace"
          "$<TARGET_FILE:vix_files_wasi_capability_test>")
add_test(NAME vix_files_wasi_functional
  COMMAND "${WASMTIME_EXECUTABLE}" run
          --dir "${_vix_files_wasi_preopen}::/workspace"
          "$<TARGET_FILE:vix_files_test>")
