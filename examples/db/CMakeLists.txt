cmake_minimum_required(VERSION 3.20)

project(vix_example_db LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_db_examples
  basic_connect
  prepared_query
  transaction
  migrations
  sqlite_basic
  mysql_basic
  pool_usage
)

set(_vix_db_runnable_examples
  basic_connect
  prepared_query
  transaction
  sqlite_basic
  pool_usage
)

enable_testing()

foreach (_example IN LISTS _vix_db_examples)
  add_executable("db_${_example}" "${_example}.cpp")
  target_compile_features("db_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("db_${_example}" PRIVATE vix::db)
endforeach()

add_executable(db_database_from_env "config/database_from_env.cpp")
target_compile_features(db_database_from_env PRIVATE cxx_std_20)
target_link_libraries(db_database_from_env PRIVATE vix::db)

foreach (_example IN LISTS _vix_db_runnable_examples)
  set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime/${_example}")
  file(MAKE_DIRECTORY "${_runtime_dir}")
  add_test(NAME "db.${_example}" COMMAND "db_${_example}")
  set_tests_properties("db.${_example}" PROPERTIES WORKING_DIRECTORY "${_runtime_dir}")
endforeach()
