cmake_minimum_required(VERSION 3.20)

project(vix_http_ws_examples LANGUAGES CXX)

set(_HTTP_WS_EXAMPLES
  main_minimal.cpp
  main_basic.cpp
  main_chat.cpp
  main_runtime.cpp
)

foreach(EXAMPLE_SRC IN LISTS _HTTP_WS_EXAMPLES)
  get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)
  set(EXAMPLE_NAME "http_ws_${_raw_name}")

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

  message(STATUS "HTTP+WS example added: ${EXAMPLE_NAME}  [${EXAMPLE_SRC}]")
endforeach()
