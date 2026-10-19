cmake_minimum_required(VERSION 3.23)
project(vix_tls_disabled_package_consumer LANGUAGES CXX)

find_package(Vix REQUIRED)
if (TARGET vix::tls_experimental)
  message(FATAL_ERROR "TLS target must not load unless explicitly requested")
endif()

add_executable(vix_tls_disabled_package_consumer main.cpp)
target_compile_features(vix_tls_disabled_package_consumer PRIVATE cxx_std_20)
target_link_libraries(vix_tls_disabled_package_consumer PRIVATE vix::vix_standard)
