cmake_minimum_required(VERSION 3.20)

project(vix_example_conversion LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_conversion_examples
  conversion_cli_config
  parse_bool
  parse_enum
  parse_float
  parse_generic
  parse_int
  to_string
  vix_conversion_showcase
)

enable_testing()

foreach (_example IN LISTS _vix_conversion_examples)
  add_executable("conversion_${_example}" "${_example}.cpp")
  target_compile_features("conversion_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("conversion_${_example}" PRIVATE vix::conversion)
  add_test(NAME "conversion.${_example}" COMMAND "conversion_${_example}")
endforeach()
