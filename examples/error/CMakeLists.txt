cmake_minimum_required(VERSION 3.20)

project(vix_example_error LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_error_examples
  basic
  chaining
  exception
  result
)

enable_testing()

foreach (_example IN LISTS _vix_error_examples)
  add_executable("error_${_example}" "${_example}.cpp")
  target_compile_features("error_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("error_${_example}" PRIVATE vix::error)
  add_test(NAME "error.${_example}" COMMAND "error_${_example}")
endforeach()
