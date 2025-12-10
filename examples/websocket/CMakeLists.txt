cmake_minimum_required(VERSION 3.20)

project(vix_websocket_examples_root LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
#  Root WebSocket examples
#
#  These are the "single file" examples living directly under
#  examples/websocket/:
#
#    • simple_server.cpp  → minimal WebSocket broadcast server
#    • simple_client.cpp  → minimal WebSocket console client
#    • chat_room.cpp      → room-based chat protocol (typed messages)
#
#  They all link against the vix::websocket module target.
# ------------------------------------------------------------------------------

set(_WS_ROOT_EXAMPLES
  simple_server.cpp
  simple_client.cpp
  chat_room.cpp
)

foreach(EXAMPLE_SRC IN LISTS _WS_ROOT_EXAMPLES)
  get_filename_component(_raw_name "${EXAMPLE_SRC}" NAME_WE)

  # Executable name: vix-ws-simple_server, vix-ws-simple_client, vix-ws-chat_room
  set(EXE_NAME "vix-ws-${_raw_name}")

  add_executable("${EXE_NAME}" "${EXAMPLE_SRC}")
  target_link_libraries("${EXE_NAME}" PRIVATE vix::websocket)

  # Optional: put binaries in the main build directory
  set_target_properties("${EXE_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  message(STATUS "WebSocket root example added: ${EXE_NAME}  [${EXAMPLE_SRC}]")
endforeach()

# ------------------------------------------------------------------------------
#  Subdirectory-based examples
#
#    • simple/   → minimal server/client + app_example/minimal_ws_server
#    • advanced/ → production-style server/client with rooms, SQLite, metrics
# ------------------------------------------------------------------------------

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/simple/CMakeLists.txt")
  add_subdirectory(simple)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/advanced/CMakeLists.txt")
  add_subdirectory(advanced)
endif()
