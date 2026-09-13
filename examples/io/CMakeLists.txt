cmake_minimum_required(VERSION 3.20)

project(vix_example_io LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_io_examples
  advanced
  basic
  buffer
  copy_stream
  read_all
  stdin
  stdout
)

set(_vix_io_runnable_examples
  basic
  buffer
  stdout
)

enable_testing()

foreach (_example IN LISTS _vix_io_examples)
  add_executable("io_${_example}" "${_example}.cpp")
  target_compile_features("io_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("io_${_example}" PRIVATE vix::io)
endforeach()

foreach (_example IN LISTS _vix_io_runnable_examples)
  add_test(NAME "io.${_example}" COMMAND "io_${_example}")
endforeach()
