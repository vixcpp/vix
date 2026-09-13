cmake_minimum_required(VERSION 3.20)

project(vix_example_kv LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_kv_examples
  basic
  compact
  delete
  hello
  list
  memory
  persistence_small
  persistent
  stats
)

enable_testing()

foreach (_example IN LISTS _vix_kv_examples)
  add_executable("kv_${_example}" "${_example}.cpp")
  target_compile_features("kv_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("kv_${_example}" PRIVATE vix::kv)

  set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime/${_example}")
  file(MAKE_DIRECTORY "${_runtime_dir}")
  add_test(NAME "kv.${_example}" COMMAND "kv_${_example}")
  set_tests_properties("kv.${_example}" PROPERTIES WORKING_DIRECTORY "${_runtime_dir}")
endforeach()
