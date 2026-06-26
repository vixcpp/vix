cmake_minimum_required(VERSION 3.20)

project(vix_requests_examples LANGUAGES CXX)

if (NOT TARGET vix::requests)
  message(STATUS "Requests examples skipped: vix::requests target is not available")
  return()
endif()

set(_REQUESTS_EXAMPLES
  01_get_with_params.cpp
  02_post_json.cpp
  03_form_login_session.cpp
  04_bearer_token.cpp
  05_download_file.cpp
  06_error_handling.cpp
  07_timeout_retry.cpp
  08_async_get.cpp
  09_paginated_api.cpp
  10_api_client_wrapper.cpp
)

foreach(EXAMPLE_SRC IN LISTS _REQUESTS_EXAMPLES)
  get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)
  set(EXAMPLE_NAME "requests_${_raw_name}")

  add_executable("${EXAMPLE_NAME}" "${EXAMPLE_SRC}")
  target_compile_features("${EXAMPLE_NAME}" PRIVATE cxx_std_20)
  target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix::requests)

  if (TARGET vix_warnings)
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_warnings)
  endif()
  if (TARGET vix_coverage)
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE vix_coverage)
  endif()

  set_target_properties("${EXAMPLE_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  message(STATUS "Requests example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
endforeach()
