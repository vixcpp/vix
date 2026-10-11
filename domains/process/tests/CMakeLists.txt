add_executable(vix_process_test process_test.cpp)
target_compile_features(vix_process_test PRIVATE cxx_std_20)
target_link_libraries(vix_process_test PRIVATE vix::process)
add_test(NAME vix_process_test COMMAND vix_process_test)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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
