cmake_minimum_required(VERSION 3.23)
project(vix_tls_package_consumer LANGUAGES CXX)

find_package(Vix REQUIRED COMPONENTS TLSExperimental)
if (NOT TARGET vix::tls_experimental)
  message(FATAL_ERROR "TLSExperimental component did not provide vix::tls_experimental")
endif()

add_executable(vix_tls_package_consumer main.cpp)
target_compile_features(vix_tls_package_consumer PRIVATE cxx_std_20)
target_link_libraries(vix_tls_package_consumer PRIVATE
  vix::tls_experimental OpenSSL::Crypto)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(vix_tls_package_consumer PRIVATE -UNDEBUG -Wall -Wextra -Werror)
endif()
