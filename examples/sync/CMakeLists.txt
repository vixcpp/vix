cmake_minimum_required(VERSION 3.20)

project(vix_example_sync LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_sync_examples
  01_outbox_basic
  02_outbox_retry_and_failure
  03_wal_append_and_replay
  04_sync_worker_success
  05_sync_worker_permanent_failure
  06_sync_engine_requeue_inflight
  07_local_write_wal_outbox_engine
  08_local_write_offline_then_recover
)

enable_testing()

foreach (_example IN LISTS _vix_sync_examples)
  add_executable("sync_${_example}" "${_example}.cpp")
  target_compile_features("sync_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("sync_${_example}" PRIVATE vix::sync)

  set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime/${_example}")
  file(MAKE_DIRECTORY "${_runtime_dir}")
  add_test(NAME "sync.${_example}" COMMAND "sync_${_example}")
  set_tests_properties("sync.${_example}" PROPERTIES WORKING_DIRECTORY "${_runtime_dir}")
endforeach()
