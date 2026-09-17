cmake_minimum_required(VERSION 3.20)

project(vix_example_tests LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_tests_examples
  assertions
  basic
  suite
)

enable_testing()

foreach (_example IN LISTS _vix_tests_examples)
  add_executable("tests_${_example}" "${_example}.cpp")
  target_compile_features("tests_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("tests_${_example}" PRIVATE vix::tests)
  add_test(NAME "tests.${_example}" COMMAND "tests_${_example}")
endforeach()

set_tests_properties(tests.assertions tests.basic PROPERTIES WILL_FAIL TRUE)
