cmake_minimum_required(VERSION 3.20)

project(vix_example_threadpool LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_threadpool_examples
  basic_post
  custom_config
  metrics
  parallel_for
  parallel_for_each
  parallel_map
  parallel_reduce
  periodic_task
  shutdown
  submit_future
  task_cancellation
  task_group
  task_priority
  task_timeout
)

set(_vix_threadpool_runnable_examples
  basic_post
  parallel_for
  parallel_for_each
  parallel_map
  parallel_reduce
  submit_future
  task_group
)

enable_testing()

foreach (_example IN LISTS _vix_threadpool_examples)
  add_executable("threadpool_${_example}" "${_example}.cpp")
  target_compile_features("threadpool_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("threadpool_${_example}" PRIVATE vix::threadpool)
endforeach()

foreach (_example IN LISTS _vix_threadpool_runnable_examples)
  add_test(NAME "threadpool.${_example}" COMMAND "threadpool_${_example}")
endforeach()
