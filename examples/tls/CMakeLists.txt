cmake_minimum_required(VERSION 3.20)

project(vix_example_tls LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

foreach (_example IN ITEMS 01_https_basic 02_https_with_config)
  add_executable("tls_${_example}" "${_example}.cpp")
  target_compile_features("tls_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("tls_${_example}" PRIVATE vix::core)
endforeach()

# HTTPS examples require user-provided certificate assets and run as build-only consumers.
