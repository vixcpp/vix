cmake_minimum_required(VERSION 3.23)
project(vix_http_tls_package_consumer LANGUAGES CXX)

find_package(Vix REQUIRED COMPONENTS HTTPSTLSExperimental)
if (NOT TARGET vix::http_tls_experimental)
  message(FATAL_ERROR "HTTPSTLSExperimental did not export vix::http_tls_experimental")
endif()
find_package(Threads REQUIRED)

get_filename_component(VIX_HTTP_SOURCE_ROOT
  "${CMAKE_CURRENT_LIST_DIR}/../../domains/http" ABSOLUTE)

add_executable(vix_https_installed_client_example
  "${VIX_HTTP_SOURCE_ROOT}/examples/https_client.cpp")
target_compile_features(vix_https_installed_client_example PRIVATE cxx_std_20)
target_link_libraries(vix_https_installed_client_example PRIVATE vix::http_tls_experimental)

add_executable(vix_https_installed_server_example
  "${VIX_HTTP_SOURCE_ROOT}/examples/https_server.cpp")
target_compile_features(vix_https_installed_server_example PRIVATE cxx_std_20)
target_link_libraries(vix_https_installed_server_example PRIVATE vix::http_tls_experimental)

add_executable(vix_https_installed_consumer
  "${VIX_HTTP_SOURCE_ROOT}/tests/https_public_api_test.cpp")
target_compile_features(vix_https_installed_consumer PRIVATE cxx_std_20)
target_link_libraries(vix_https_installed_consumer PRIVATE
  vix::http_tls_experimental Threads::Threads OpenSSL::Crypto)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(vix_https_installed_client_example PRIVATE -Wall -Wextra -Werror)
  target_compile_options(vix_https_installed_server_example PRIVATE -Wall -Wextra -Werror)
  target_compile_options(vix_https_installed_consumer PRIVATE -Wall -Wextra -Werror)
endif()
