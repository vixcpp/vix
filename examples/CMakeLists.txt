cmake_minimum_required(VERSION 3.23)

project(vix_platform_examples LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

function(vix_add_example target source library)
  if (NOT TARGET "${library}")
    message(FATAL_ERROR "The installed Vix SDK does not provide ${library}, required by ${target}.")
  endif()

  add_executable("${target}" "${source}")
  target_compile_features("${target}" PRIVATE cxx_std_20)
  target_link_libraries("${target}" PRIVATE "${library}")
endfunction()

vix_add_example(standard_result standard/result.cpp vix::vix_standard)
vix_add_example(standard_time standard/time.cpp vix::vix_standard)
vix_add_example(standard_cancellation standard/cancellation.cpp vix::vix_standard)

vix_add_example(concurrency_submit concurrency/submit_work.cpp vix::vix_concurrency)
vix_add_example(concurrency_notifications concurrency/notifications.cpp vix::vix_concurrency)
vix_add_example(concurrency_cancel_deadline concurrency/cancel_deadline.cpp vix::vix_concurrency)

vix_add_example(files_read_write files/read_write.cpp vix::vix_files)
vix_add_example(files_iterate_directory files/iterate_directory.cpp vix::vix_files)
vix_add_example(files_copy_move files/copy_move.cpp vix::vix_files)

vix_add_example(process_run_command process/run_command.cpp vix::vix_process)
vix_add_example(process_capture_output process/capture_output.cpp vix::vix_process)
vix_add_example(process_controlled_wait process/controlled_wait.cpp vix::vix_process)

if (TARGET vix::vix_network)
  vix_add_example(network_cooperative network/cooperative.cpp vix::vix_network)
  vix_add_example(network_tcp_client network/tcp_client.cpp vix::vix_network)
  vix_add_example(network_tcp_server network/tcp_server.cpp vix::vix_network)
else()
  message(STATUS "Vix Network examples are omitted because this SDK has no Network target.")
endif()
