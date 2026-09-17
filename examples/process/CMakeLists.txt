cmake_minimum_required(VERSION 3.20)

project(vix_example_process LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_process_examples
  basic_spawn
  wait_process
  capture_output
  check_status
  kill_process
  terminate_process
  pipeline_output
  pipeline_basic
  output_large
)

foreach (_example IN LISTS _vix_process_examples)
  add_executable("process_${_example}" "${_example}.cpp")
  target_compile_features("process_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("process_${_example}" PRIVATE vix::process)
endforeach()

foreach (_example IN ITEMS async_output async_spawn_wait async_cancel async_pipeline)
  add_executable("process_${_example}" "async/${_example}.cpp")
  target_compile_features("process_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("process_${_example}" PRIVATE vix::process vix::async)
endforeach()

# These two examples select their shell command for Unix or Windows at compile time.
enable_testing()
foreach (_example IN ITEMS pipeline_output output_large)
  add_test(NAME "process.${_example}" COMMAND "process_${_example}")
endforeach()
