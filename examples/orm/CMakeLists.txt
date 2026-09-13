cmake_minimum_required(VERSION 3.20)

project(vix_example_orm LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_orm_examples
  01_basic_repository
  02_repository_crud
  03_find_all_and_count
  04_exists_and_delete
  05_unit_of_work
  06_batch_insert_tx
  07_query_builder_select
  08_query_builder_update
  09_error_handling
  10_custom_repository
  11_sqlite_repository
  12_mysql_repository
  13_migrations_code
  14_migrations_files
  15_entity_identity
  16_manual_sql_with_orm
)

set(_vix_orm_runnable_examples
  01_basic_repository
  02_repository_crud
  03_find_all_and_count
  04_exists_and_delete
  05_unit_of_work
  06_batch_insert_tx
  07_query_builder_select
  08_query_builder_update
  09_error_handling
  10_custom_repository
  11_sqlite_repository
  13_migrations_code
  14_migrations_files
  15_entity_identity
  16_manual_sql_with_orm
)

enable_testing()

foreach (_example IN LISTS _vix_orm_examples)
  add_executable("orm_${_example}" "${_example}.cpp")
  target_compile_features("orm_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("orm_${_example}" PRIVATE vix::orm)
endforeach()

foreach (_example IN LISTS _vix_orm_runnable_examples)
  set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime/${_example}")
  file(MAKE_DIRECTORY "${_runtime_dir}")

  if (_example STREQUAL "14_migrations_files")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/migrations" DESTINATION "${_runtime_dir}")
  endif()

  add_test(NAME "orm.${_example}" COMMAND "orm_${_example}")
  set_tests_properties("orm.${_example}" PROPERTIES WORKING_DIRECTORY "${_runtime_dir}")

  if (_example STREQUAL "09_error_handling")
    set_tests_properties("orm.${_example}" PROPERTIES WILL_FAIL TRUE)
  endif()
endforeach()
