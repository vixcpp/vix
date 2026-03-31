cmake_minimum_required(VERSION 3.20)

project(vix_http_crud_examples LANGUAGES CXX)

set(_HTTP_CRUD_EXAMPLES
  delete_user.cpp
  post_create_user.cpp
  put_update_user.cpp
  user_crud_with_validation.cpp
  users_crud_internal.cpp
  users_crud.cpp
  repository_crud_full.cpp
)

foreach(EXAMPLE_SRC IN LISTS _HTTP_CRUD_EXAMPLES)
  get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)
  set(EXAMPLE_NAME "http_crud_${_raw_name}")

  add_executable("${EXAMPLE_NAME}" "${EXAMPLE_SRC}")
  target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix::vix)

  if (TARGET vix_warnings)
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_warnings)
  endif()
  if (TARGET vix_coverage)
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_coverage)
  endif()

  set_target_properties("${EXAMPLE_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  message(STATUS "HTTP CRUD example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
endforeach()
