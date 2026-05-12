#  @file examples/CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2025, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/vix
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix.cpp - Threadpool Examples
# ====================================================================

set(VIX_THREADPOOL_EXAMPLE_SOURCES
  basic_post.cpp
  custom_config.cpp
  metrics.cpp
  parallel_for.cpp
  parallel_for_each.cpp
  parallel_map.cpp
  parallel_reduce.cpp
  periodic_task.cpp
  shutdown.cpp
  submit_future.cpp
  task_cancellation.cpp
  task_group.cpp
  task_priority.cpp
  task_timeout.cpp
)

foreach(example_source IN LISTS VIX_THREADPOOL_EXAMPLE_SOURCES)
  get_filename_component(example_name "${example_source}" NAME_WE)

  add_executable(${example_name}
    ${example_source}
  )

  target_link_libraries(${example_name}
    PRIVATE
      vix::threadpool
  )

  target_compile_features(${example_name}
    PRIVATE
      cxx_std_20
  )

  if (NOT MSVC)
    target_compile_options(${example_name}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
  endif()
endforeach()
