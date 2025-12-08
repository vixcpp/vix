# ------------------------------------------------------------------------------
#  CMakeLists.txt — WebSocket examples entrypoint
#
#  This file aggregates all example targets for the vix::websocket module:
#
#    • examples/simple/   — minimal server/client for quick start
#    • examples/advanced/ — production-style server/client with:
#         - rooms, persistence (SQLite), history replay, metrics,
#           auto-reconnect, and typed event routing
#
#  It is included from the top-level websocket CMakeLists.txt via:
#
#      option(VIX_WEBSOCKET_BUILD_EXAMPLES ...)
#      if (VIX_WEBSOCKET_BUILD_EXAMPLES)
#          add_subdirectory(examples)
#      endif()
#
#  This keeps examples fully optional and avoids polluting consumers’
#  target namespaces in production builds.
# ------------------------------------------------------------------------------

add_subdirectory(simple)
add_subdirectory(advanced)
