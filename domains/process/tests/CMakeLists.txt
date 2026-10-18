# Assertions must remain active in test binaries, including Release.
# Production libraries retain their normal NDEBUG configuration.
if(MSVC)
  add_compile_options(/UNDEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-UNDEBUG)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
  if (NOT DEFINED WASMTIME_EXECUTABLE OR NOT EXISTS "${WASMTIME_EXECUTABLE}")
    message(FATAL_ERROR "Set WASMTIME_EXECUTABLE; the WASI unsupported-process contract test must execute.")
  endif()
  add_executable(vix_process_wasi_unsupported_test process_wasi_unsupported_test.cpp)
  target_compile_features(vix_process_wasi_unsupported_test PRIVATE cxx_std_20)
  target_link_libraries(vix_process_wasi_unsupported_test PRIVATE vix::process)
  add_test(NAME vix_process_wasi_spawn_unsupported
    COMMAND "${WASMTIME_EXECUTABLE}" run "$<TARGET_FILE:vix_process_wasi_unsupported_test>")
else()
  add_executable(vix_process_test_child process_test_child.cpp)
  target_compile_features(vix_process_test_child PRIVATE cxx_std_20)
  add_executable(vix_process_test process_test.cpp)
  add_dependencies(vix_process_test vix_process_test_child)
  target_compile_features(vix_process_test PRIVATE cxx_std_20)
  target_link_libraries(vix_process_test PRIVATE vix::process)
  if (NOT WIN32)
    target_link_libraries(vix_process_test PRIVATE Threads::Threads)
  endif()
  add_test(NAME vix_process_test COMMAND vix_process_test)
  set_tests_properties(vix_process_test PROPERTIES TIMEOUT 45)
  if (ANDROID)
    set_tests_properties(vix_process_test PROPERTIES LABELS "process;android;functional")
  endif()
endif()

add_executable(vix_process_public_consumer process_public_consumer.cpp)
target_compile_features(vix_process_public_consumer PRIVATE cxx_std_20)
target_link_libraries(vix_process_public_consumer PRIVATE vix::process)
if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_test(NAME vix_process_public_consumer
    COMMAND "${WASMTIME_EXECUTABLE}" run "$<TARGET_FILE:vix_process_public_consumer>")
else()
  add_test(NAME vix_process_public_consumer COMMAND vix_process_public_consumer)
endif()
set_tests_properties(vix_process_public_consumer PROPERTIES TIMEOUT 15)

if (NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WASI" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Compile the actual POSIX provider with exceptions disabled as well as the
  # public header probe.  It is an object target so it adds no second library.
  add_library(vix_process_noexcept_provider_probe OBJECT ../src/ProcessPosix.cpp)
  target_compile_features(vix_process_noexcept_provider_probe PRIVATE cxx_std_20)
  target_include_directories(vix_process_noexcept_provider_probe PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)
  target_link_libraries(vix_process_noexcept_provider_probe PRIVATE vix::standard vix::files Threads::Threads)
  target_compile_options(vix_process_noexcept_provider_probe PRIVATE -fno-exceptions)

  try_compile(vix_process_exceptions_disabled_compiled
    "${CMAKE_CURRENT_BINARY_DIR}/exceptions_disabled"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/exceptions_disabled.cpp"
    COMPILE_DEFINITIONS "-fno-exceptions"
    CMAKE_FLAGS
      "-DCMAKE_CXX_STANDARD=20"
      "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
      "-DCMAKE_TRY_COMPILE_TARGET_TYPE:STRING=STATIC_LIBRARY"
      "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_CURRENT_LIST_DIR}/../include\\;${CMAKE_CURRENT_LIST_DIR}/../../files/include\\;${CMAKE_CURRENT_LIST_DIR}/../../../standard/include"
    OUTPUT_VARIABLE vix_process_exceptions_disabled_output)
  if (NOT vix_process_exceptions_disabled_compiled)
    message(FATAL_ERROR "Process public headers must compile with exceptions disabled\n${vix_process_exceptions_disabled_output}")
  endif()
endif()
