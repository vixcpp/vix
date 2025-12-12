# cmake_minimum_required(VERSION 3.20)

# project(vix_examples_root LANGUAGES CXX)

# # This CMakeLists is responsible for subdirectories under examples/
# # that contain their own example groups (http, http_crud, ...).
# #
# # The root CMakeLists.txt must call:
# #   add_subdirectory(examples)
# # when VIX_BUILD_EXAMPLES is ON.

# # HTTP-only examples
# if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/http/CMakeLists.txt")
#   message(STATUS "Examples: enabling HTTP examples (examples/http)")
#   add_subdirectory(http)
# endif()

# # HTTP CRUD examples
# if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/http_crud/CMakeLists.txt")
#   message(STATUS "Examples: enabling HTTP CRUD examples (examples/http_crud)")
#   add_subdirectory(http_crud)
# endif()
