cmake_minimum_required(VERSION 3.20)

project(vix_example_async LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_async_examples
  00_hello_task
  01_signal_stop
  01_timer
  02_thread_pool
  03_tcp_echo_server
)

set(_vix_async_runnable_examples
  00_hello_task
  01_timer
  02_thread_pool
)

set(_vix_async_console_examples
  00_hello_task
  01_signal_stop
  01_timer
  02_thread_pool
  03_tcp_echo_server
)

enable_testing()

foreach (_example IN LISTS _vix_async_examples)
  add_executable("async_${_example}" "${_example}.cpp")
  target_compile_features("async_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("async_${_example}" PRIVATE vix::async)

  if (_example IN_LIST _vix_async_console_examples)
    target_link_libraries("async_${_example}" PRIVATE vix::io)
  endif()
endforeach()

foreach (_example IN LISTS _vix_async_runnable_examples)
  add_test(NAME "async.${_example}" COMMAND "async_${_example}")
endforeach()
