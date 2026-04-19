#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
#  Vix.cpp
add_executable(vix_process_basic_spawn
  basic_spawn.cpp
)
target_link_libraries(vix_process_basic_spawn PRIVATE vix::process)

add_executable(vix_process_wait_process
  wait_process.cpp
)
target_link_libraries(vix_process_wait_process PRIVATE vix::process)

add_executable(vix_process_capture_output
  capture_output.cpp
)
target_link_libraries(vix_process_capture_output PRIVATE vix::process)

add_executable(vix_process_check_status
  check_status.cpp
)
target_link_libraries(vix_process_check_status PRIVATE vix::process)

add_executable(vix_process_kill_process
  kill_process.cpp
)
target_link_libraries(vix_process_kill_process PRIVATE vix::process)

add_executable(vix_process_terminate_process
  terminate_process.cpp
)
target_link_libraries(vix_process_terminate_process PRIVATE vix::process)

add_executable(vix_process_pipeline_output
  pipeline_output.cpp
)

target_link_libraries(vix_process_pipeline_output
  PRIVATE
    vix::process
)

add_executable(vix_process_pipeline_basic
  pipeline_basic.cpp
)
target_link_libraries(vix_process_pipeline_basic PRIVATE vix::process)

add_executable(vix_process_output_large
  output_large.cpp
)
target_link_libraries(vix_process_output_large PRIVATE vix::process)

# async examples
add_subdirectory(async)
