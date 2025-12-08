# ------------------------------------------------------------------------------
#  CMakeLists.txt — Simple WebSocket Examples for Vix.cpp
#
#  This CMake file builds two minimal demonstration executables showcasing
#  the fundamentals of the Vix.cpp WebSocket module:
#
#    1. vix-ws-simple-server  — a minimal broadcast-only WebSocket server
#       (source: src/simple_server.cpp)
#
#    2. vix-ws-simple-client  — a minimal interactive WebSocket client
#       (source: src/simple_client.cpp)
#
#  These examples are intentionally lightweight and focus on essential concepts:
#
#    • Loading configuration (server example)
#    • Initializing a WebSocket server with minimal handlers
#    • Connecting a client to localhost:9090
#    • Sending and receiving typed JSON messages ("type" + "payload")
#    • Handling lifecycle events: open, message, close, error
#
#  Both executables link against the umbrella `vix::websocket` target,
#  which bundles:
#      - WebSocket server + client APIs
#      - Session management
#      - Typed JSON protocol helpers
#      - Broadcast utilities
#
#  This directory is ideal for:
#    • Quick-start learning
#    • Basic testing during development
#    • Prototyping simple real-time features
#
#  For room-based messaging, persistence (SQLite), metrics,
#  auto-reconnect, and history replay, see:
#      examples/advanced/
#
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
#  Simple WebSocket Server
# ------------------------------------------------------------------------------

add_executable(vix-ws-simple-server
    src/simple_server.cpp
)

target_link_libraries(vix-ws-simple-server
    PRIVATE
        vix::websocket
)

add_executable(vix-ws-simple-app
    src/app_example.cpp
)

target_link_libraries(vix-ws-simple-app
    PRIVATE
        vix::websocket
)

add_executable(vix-ws-minimal-server
    src/minimal_ws_server.cpp
)

target_link_libraries(vix-ws-minimal-server
    PRIVATE
        vix::websocket
)

# ------------------------------------------------------------------------------
#  Simple WebSocket Client
# ------------------------------------------------------------------------------

add_executable(vix-ws-simple-client
    src/simple_client.cpp
)

target_link_libraries(vix-ws-simple-client
    PRIVATE
        vix::websocket
)
