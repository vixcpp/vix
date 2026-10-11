add_executable(vix_files_test files_test.cpp)
target_compile_features(vix_files_test PRIVATE cxx_std_20)
target_link_libraries(vix_files_test PRIVATE vix::files)
add_test(NAME vix_files_test COMMAND vix_files_test)

function(vix_files_expect_exception_disabled_compile name source)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    return()
  endif()
  try_compile(${name}_compiled
    "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    COMPILE_DEFINITIONS "-fno-exceptions"
    CMAKE_FLAGS
      "-DCMAKE_CXX_STANDARD=20"
      "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
      "-DCMAKE_TRY_COMPILE_TARGET_TYPE:STRING=STATIC_LIBRARY"
      "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_LIST_DIR}/../include;${CMAKE_CURRENT_LIST_DIR}/../../../standard/include"
    OUTPUT_VARIABLE ${name}_output)
  if (NOT ${name}_compiled)
    message(FATAL_ERROR "${source} must compile with exceptions disabled.\n${${name}_output}")
  endif()
endfunction()

vix_files_expect_exception_disabled_compile(vix_files_exceptions_disabled exceptions_disabled.cpp)
