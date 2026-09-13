cmake_minimum_required(VERSION 3.20)

project(vix_example_session LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

add_executable(session_session session.cpp)
target_compile_features(session_session PRIVATE cxx_std_20)
target_link_libraries(session_session PRIVATE vix::middleware)
