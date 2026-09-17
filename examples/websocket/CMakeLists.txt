cmake_minimum_required(VERSION 3.20)

project(vix_example_websocket LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

# Keep the example source surface explicit.  The consumer CI may place its
# build directory below this source directory; recursive discovery would then
# capture CMake-generated CompilerId sources as WebSocket examples.
set(_vix_websocket_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/01_minimal_ws/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/02_http_ws_basic/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/03_chat_rooms/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/04_chat_persistent/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/05_chat_long_polling/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/06_metrics_runtime/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/07_client_runtime/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/08_realtime_dashboard/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/advanced/src/client.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/advanced/src/server.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/chat_room.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/minimal_ws_server.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple/src/app_example.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple/src/minimal_ws_server.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple/src/simple_client.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple/src/simple_server.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple_client.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple_server.cpp"
)

foreach (_source IN LISTS _vix_websocket_sources)
  file(RELATIVE_PATH _relative "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
  string(REPLACE "/" "_" _name "${_relative}")
  string(REPLACE "\\" "_" _name "${_name}")
  string(REGEX REPLACE "\\.cpp$" "" _name "${_name}")
  add_executable("websocket_${_name}" "${_source}")
  target_compile_features("websocket_${_name}" PRIVATE cxx_std_20)
  target_link_libraries("websocket_${_name}" PRIVATE vix::websocket)
endforeach()
