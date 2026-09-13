cmake_minimum_required(VERSION 3.20)

project(vix_example_config LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

function(vix_add_config_example name)
  add_executable("config_${name}" "${name}.cpp")
  target_compile_features("config_${name}" PRIVATE cxx_std_20)
  target_link_libraries("config_${name}" PRIVATE vix::core)
endfunction()

vix_add_config_example(01_server_port)
vix_add_config_example(02_database_config)
target_link_libraries(config_02_database_config PRIVATE vix::db)
vix_add_config_example(03_logging_config)
vix_add_config_example(04_websocket_config)
target_link_libraries(config_04_websocket_config PRIVATE vix::websocket)
vix_add_config_example(05_layered_env)
target_link_libraries(config_05_layered_env PRIVATE vix::env)
vix_add_config_example(06_database_from_env)
target_link_libraries(config_06_database_from_env PRIVATE vix::db)
