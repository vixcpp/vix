cmake_minimum_required(VERSION 3.20)

project(vix_example_time LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_time_examples
  data
  datetime
  steady_clock
  time
  timestamp_duration
)

enable_testing()

foreach (_example IN LISTS _vix_time_examples)
  add_executable("time_${_example}" "${_example}.cpp")
  target_compile_features("time_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("time_${_example}" PRIVATE vix::time)
  add_test(NAME "time.${_example}" COMMAND "time_${_example}")
endforeach()
