cmake_minimum_required(VERSION 3.20)

project(vix_example_log LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_log_examples
  basic
  context
  format
)

enable_testing()

foreach (_example IN LISTS _vix_log_examples)
  add_executable("log_${_example}" "${_example}.cpp")
  target_compile_features("log_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("log_${_example}" PRIVATE vix::log)
  add_test(NAME "log.${_example}" COMMAND "log_${_example}")
endforeach()
