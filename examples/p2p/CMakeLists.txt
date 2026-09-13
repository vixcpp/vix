cmake_minimum_required(VERSION 3.20)

project(vix_example_p2p LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_p2p_examples
  01_envelope_and_framing_basic
  02_hello_handshake_messages
  03_discovery_announce_json
  04_router_memory_basic
  05_pack_secure_envelope
  06_dispatch_decode_basic
  07_wal_push_and_ack
  08_runtime_manual_connect
  09_udp_discovery_basic
  10_bootstrap_http_basic
)

foreach (_example IN LISTS _vix_p2p_examples)
  add_executable("p2p_${_example}" "${_example}.cpp")
  target_compile_features("p2p_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("p2p_${_example}" PRIVATE vix::p2p)
endforeach()

add_executable(p2p_web_server web/server.cpp)
target_compile_features(p2p_web_server PRIVATE cxx_std_20)
target_link_libraries(p2p_web_server PRIVATE vix::p2p vix::p2p_http)

enable_testing()
foreach (_example IN LISTS _vix_p2p_examples)
  if (_example MATCHES "^0[1-7]_")
    add_test(NAME "p2p.${_example}" COMMAND "p2p_${_example}")
  endif()
endforeach()
