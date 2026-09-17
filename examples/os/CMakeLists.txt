cmake_minimum_required(VERSION 3.20)

project(vix_example_os LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_os_examples
  basic
  sleep
  system_info
  user_info
)

enable_testing()

foreach (_example IN LISTS _vix_os_examples)
  add_executable("os_${_example}" "${_example}.cpp")
  target_compile_features("os_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("os_${_example}" PRIVATE vix::os)
  add_test(NAME "os.${_example}" COMMAND "os_${_example}")
endforeach()
