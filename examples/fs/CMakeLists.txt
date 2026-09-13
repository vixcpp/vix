cmake_minimum_required(VERSION 3.20)

project(vix_example_fs LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_fs_examples
  advanced
  basic
  copy_move
  filesystem
  list
  read_write
)

enable_testing()

foreach (_example IN LISTS _vix_fs_examples)
  add_executable("fs_${_example}" "${_example}.cpp")
  target_compile_features("fs_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("fs_${_example}" PRIVATE vix::fs)
  add_test(NAME "fs.${_example}" COMMAND "fs_${_example}")
endforeach()
