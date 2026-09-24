cmake_minimum_required(VERSION 3.20)
project(vix_example_auth LANGUAGES CXX)
find_package(Vix CONFIG REQUIRED)
# Keep the example source surface explicit.  The consumer CI intentionally
# places its build directory below the copied source directory; recursive
# discovery would otherwise turn CMake's generated CompilerId sources into
# example executables.
set(_vix_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/api_key_app_simple.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/generate_token/jwt_gen.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/jwt/jwt_app_simple.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/jwt/jwt_pipeline_example.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/rbac/rbac_app_simple.cpp"
)
# Documentation fragment: session_cookies.cpp intentionally has no main().
list(REMOVE_ITEM _vix_sources "${CMAKE_CURRENT_SOURCE_DIR}/session_cookies.cpp")
foreach (_source IN LISTS _vix_sources)
  file(RELATIVE_PATH _relative "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
  string(REPLACE "/" "_" _name "${_relative}")
  string(REGEX REPLACE "\\.cpp$" "" _name "${_name}")
  add_executable("auth_${_name}" "${_source}")
  target_compile_features("auth_${_name}" PRIVATE cxx_std_20)
  target_link_libraries("auth_${_name}" PRIVATE vix::middleware)
endforeach()
