add_executable(vix_standard_target_smoke_test
  standard_target_smoke_test.cpp
)

target_compile_features(vix_standard_target_smoke_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_target_smoke_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_target_smoke_test
  COMMAND vix_standard_target_smoke_test
)

add_executable(vix_standard_result_test
  result_test.cpp
)

target_compile_features(vix_standard_result_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_result_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_result_test
  COMMAND vix_standard_result_test
)

add_executable(vix_standard_error_test
  error_test.cpp
)

target_compile_features(vix_standard_error_test PRIVATE cxx_std_20)

find_package(Threads REQUIRED)

target_link_libraries(vix_standard_error_test
  PRIVATE
    vix::standard
    Threads::Threads
)

add_test(
  NAME vix_standard_error_test
  COMMAND vix_standard_error_test
)

add_executable(vix_standard_result_fault_test
  result_fault_test.cpp
)

target_compile_features(vix_standard_result_fault_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_result_fault_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_result_value_fault_test
  COMMAND vix_standard_result_fault_test value
)

add_test(
  NAME vix_standard_result_error_fault_test
  COMMAND vix_standard_result_fault_test error
)

add_test(
  NAME vix_standard_result_taken_value_fault_test
  COMMAND vix_standard_result_fault_test taken_value
)

add_test(
  NAME vix_standard_result_second_take_fault_test
  COMMAND vix_standard_result_fault_test second_take
)

add_executable(vix_standard_time_test
  time_test.cpp
)

target_compile_features(vix_standard_time_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_time_test
  PRIVATE
    vix::standard
    Threads::Threads
)

add_test(
  NAME vix_standard_time_test
  COMMAND vix_standard_time_test
)

add_executable(vix_standard_time_fault_test
  time_fault_test.cpp
)

target_compile_features(vix_standard_time_fault_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_time_fault_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_time_duration_fault_test
  COMMAND vix_standard_time_fault_test duration
)

add_test(
  NAME vix_standard_time_deadline_fault_test
  COMMAND vix_standard_time_fault_test deadline
)

add_executable(vix_standard_cancellation_test
  cancellation_test.cpp
)

target_compile_features(vix_standard_cancellation_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_cancellation_test
  PRIVATE
    vix::standard
    Threads::Threads
)

add_test(
  NAME vix_standard_cancellation_test
  COMMAND vix_standard_cancellation_test
)

add_executable(vix_standard_cancellation_fault_test
  cancellation_fault_test.cpp
)

target_compile_features(vix_standard_cancellation_fault_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_cancellation_fault_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_cancellation_callback_fault_test
  COMMAND vix_standard_cancellation_fault_test
)

add_executable(vix_standard_completion_test
  completion_test.cpp
)

target_compile_features(vix_standard_completion_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_completion_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_completion_test
  COMMAND vix_standard_completion_test
)

add_executable(vix_standard_completion_fault_test
  completion_fault_test.cpp
)

target_compile_features(vix_standard_completion_fault_test PRIVATE cxx_std_20)

target_link_libraries(vix_standard_completion_fault_test
  PRIVATE
    vix::standard
)

add_test(
  NAME vix_standard_completion_value_fault_test
  COMMAND vix_standard_completion_fault_test value
)

add_test(
  NAME vix_standard_completion_error_fault_test
  COMMAND vix_standard_completion_fault_test error
)

add_test(
  NAME vix_standard_completion_taken_value_fault_test
  COMMAND vix_standard_completion_fault_test taken_value
)

add_test(
  NAME vix_standard_completion_second_take_fault_test
  COMMAND vix_standard_completion_fault_test second_take
)

set_tests_properties(
  vix_standard_result_value_fault_test
  vix_standard_result_error_fault_test
  vix_standard_result_taken_value_fault_test
  vix_standard_result_second_take_fault_test
  vix_standard_time_duration_fault_test
  vix_standard_time_deadline_fault_test
  vix_standard_completion_value_fault_test
  vix_standard_completion_error_fault_test
  vix_standard_completion_taken_value_fault_test
  vix_standard_completion_second_take_fault_test
  vix_standard_cancellation_callback_fault_test
  PROPERTIES WILL_FAIL TRUE
)

# Repository verification convenience only.  This is deliberately not a
# dependency of vix::standard: consumers receive the header-only programming
# model, while the repository can explicitly build its executable checks.
add_custom_target(vix_standard_check
  DEPENDS
    vix_standard_target_smoke_test
    vix_standard_result_test
    vix_standard_error_test
    vix_standard_result_fault_test
    vix_standard_time_test
    vix_standard_time_fault_test
    vix_standard_cancellation_test
    vix_standard_cancellation_fault_test
    vix_standard_completion_test
    vix_standard_completion_fault_test
)

function(vix_standard_expect_compile_failure name source)
  try_compile(${name}_compiled
    "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    CMAKE_FLAGS
      "-DCMAKE_CXX_STANDARD=20"
      "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
      "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_LIST_DIR}/../include"
    OUTPUT_VARIABLE ${name}_output
  )

  if (${name}_compiled)
    message(FATAL_ERROR
      "${source} unexpectedly compiled; ErrorIdentity must reject this "
      "invalid constant definition.\n${${name}_output}")
  endif()
endfunction()

function(vix_standard_expect_exception_disabled_compile name source)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    return()
  endif()

  try_compile(${name}_compiled
    "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    CMAKE_FLAGS
      "-DCMAKE_CXX_STANDARD=20"
      "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
      "-DCMAKE_CXX_FLAGS=-fno-exceptions"
      "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_LIST_DIR}/../include"
    OUTPUT_VARIABLE ${name}_output
  )

  if (NOT ${name}_compiled)
    message(FATAL_ERROR
      "${source} must compile with exceptions disabled.\n${${name}_output}")
  endif()
endfunction()

vix_standard_expect_compile_failure(
  vix_standard_empty_error_domain
  invalid_empty_error_domain.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_zero_error_code
  invalid_zero_error_code.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_result_reference_value
  invalid_result_reference_value.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_result_const_value
  invalid_result_const_value.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_result_volatile_value
  invalid_result_volatile_value.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_result_error_value
  invalid_result_error_value.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_clock_domain_compare
  invalid_clock_domain_compare.cpp
)

vix_standard_expect_compile_failure(
  vix_standard_clock_domain_subtract
  invalid_clock_domain_subtract.cpp
)

vix_standard_expect_exception_disabled_compile(
  vix_standard_error_exceptions_disabled
  error_exceptions_disabled.cpp
)

vix_standard_expect_exception_disabled_compile(
  vix_standard_result_exceptions_disabled
  result_exceptions_disabled.cpp
)

vix_standard_expect_exception_disabled_compile(
  vix_standard_completion_exceptions_disabled
  completion_exceptions_disabled.cpp
)

vix_standard_expect_exception_disabled_compile(
  vix_standard_time_exceptions_disabled
  time_exceptions_disabled.cpp
)

vix_standard_expect_exception_disabled_compile(
  vix_standard_cancellation_exceptions_disabled
  cancellation_exceptions_disabled.cpp
)
