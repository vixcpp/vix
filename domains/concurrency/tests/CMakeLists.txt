# Assertions must remain active in test binaries, including Release.
# Production libraries retain their normal NDEBUG configuration.
if(MSVC)
  add_compile_options(/UNDEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-UNDEBUG)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
  if (NOT VIX_WASMTIME_EXECUTABLE OR NOT EXISTS "${VIX_WASMTIME_EXECUTABLE}")
    message(FATAL_ERROR "WASI Concurrency capability test requires VIX_WASMTIME_EXECUTABLE.")
  endif()
  add_executable(vix_concurrency_wasi_unsupported_test wasi_unsupported_test.cpp)
  target_compile_features(vix_concurrency_wasi_unsupported_test PRIVATE cxx_std_20)
  target_link_libraries(vix_concurrency_wasi_unsupported_test PRIVATE vix_concurrency)
  add_test(NAME vix_concurrency_wasi_execution_unsupported
    COMMAND "${VIX_WASMTIME_EXECUTABLE}" run "$<TARGET_FILE:vix_concurrency_wasi_unsupported_test>")
else()
  add_executable(vix_concurrency_execution_resource_test execution_resource_test.cpp)
  target_compile_features(vix_concurrency_execution_resource_test PRIVATE cxx_std_20)
  target_link_libraries(vix_concurrency_execution_resource_test PRIVATE vix_concurrency Threads::Threads)
  add_test(NAME vix_concurrency_execution_resource_test
    COMMAND vix_concurrency_execution_resource_test)

endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
  add_library(vix_concurrency_no_exceptions_compile OBJECT exception_disabled_compile.cpp)
  target_compile_features(vix_concurrency_no_exceptions_compile PRIVATE cxx_std_20)
  target_link_libraries(vix_concurrency_no_exceptions_compile PRIVATE vix_concurrency)
  target_compile_options(vix_concurrency_no_exceptions_compile PRIVATE -fno-exceptions)
endif()
