# async examples for vix::process

add_executable(vix_process_async_output
  async_output.cpp
)

target_link_libraries(vix_process_async_output
  PRIVATE
    vix::process
    vix::async
)

add_executable(vix_process_async_spawn_wait
  async_spawn_wait.cpp
)

target_link_libraries(vix_process_async_spawn_wait
  PRIVATE
    vix::process
    vix::async
)

add_executable(vix_process_async_cancel
  async_cancel.cpp
)

target_link_libraries(vix_process_async_cancel
  PRIVATE
    vix::process
    vix::async
)

add_executable(vix_process_async_pipeline
  async_pipeline.cpp
)

target_link_libraries(vix_process_async_pipeline
  PRIVATE
    vix::process
    vix::async
)
