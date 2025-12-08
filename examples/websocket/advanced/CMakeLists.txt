# ------------------------------------------------------------------------------
#  CMakeLists.txt — Advanced WebSocket Examples for Vix.cpp
#
#  This CMake file builds two advanced demonstration executables showcasing
#  production-style usage of the Vix.cpp WebSocket module:
#
#    1. vix-ws-advanced-server  — a room-based, persistent WebSocket server with:
#         - typed JSON protocol ("type" + "payload")
#         - room join/leave/broadcast
#         - SQLite-backed message persistence + history replay
#         - Prometheus-style /metrics endpoint
#
#    2. vix-ws-advanced-client  — an interactive CLI WebSocket client with:
#         - auto-reconnect
#         - heartbeat / ping
#         - room switching and structured output
#
#  Both executables link only against the umbrella `vix::websocket` target.
#  All transitive dependencies (core, utils, Boost, SQLite, etc.) are provided
#  by the module itself, keeping these examples clean and focused on behavior,
#  not build plumbing.
#
#  For quick-start / minimal usage, see:
#      examples/simple/
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(vix_ws_advanced LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
#  Advanced WebSocket Server
# ------------------------------------------------------------------------------

add_executable(vix-ws-advanced-server
    src/server.cpp
)

target_link_libraries(vix-ws-advanced-server
    PRIVATE
        vix::websocket
)

# ------------------------------------------------------------------------------
#  Advanced WebSocket Client
# ------------------------------------------------------------------------------

add_executable(vix-ws-advanced-client
    src/client.cpp
)

target_link_libraries(vix-ws-advanced-client
    PRIVATE
        vix::websocket
)
