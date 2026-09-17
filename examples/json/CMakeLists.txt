cmake_minimum_required(VERSION 3.20)

project(vix_example_json LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_json_examples
  builders
  io
  jpath
  quick_start
)

set(_vix_json_simple_examples
  01_basic_values
  02_arrays
  03_objects
  04_nested
  05_mutation
  06_iteration
  07_merge_and_erase
  08_to_nlohmann_json
)

enable_testing()

foreach (_example IN LISTS _vix_json_examples)
  add_executable("json_${_example}" "${_example}.cpp")
  target_compile_features("json_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("json_${_example}" PRIVATE vix::json)
  add_test(NAME "json.${_example}" COMMAND "json_${_example}")
endforeach()

foreach (_example IN LISTS _vix_json_simple_examples)
  add_executable("json_simple_${_example}" "json_simple/${_example}.cpp")
  target_compile_features("json_simple_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("json_simple_${_example}" PRIVATE vix::json)
  add_test(NAME "json.simple.${_example}" COMMAND "json_simple_${_example}")
endforeach()
