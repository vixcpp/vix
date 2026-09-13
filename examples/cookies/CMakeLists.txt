cmake_minimum_required(VERSION 3.20)

project(vix_example_cookies LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

add_executable(cookies_cookie cookie.cpp)
target_compile_features(cookies_cookie PRIVATE cxx_std_20)
target_link_libraries(cookies_cookie PRIVATE vix::middleware)
