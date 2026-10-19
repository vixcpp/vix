add_executable(vix_http_p1_protocol_test
  http1_protocol_test.cpp
  ../src/Http1.cpp
)
target_compile_features(vix_http_p1_protocol_test PRIVATE cxx_std_20)
target_include_directories(vix_http_p1_protocol_test PRIVATE ../src)
add_test(NAME vix_http_p1_protocol_test COMMAND vix_http_p1_protocol_test)
set_tests_properties(vix_http_p1_protocol_test PROPERTIES TIMEOUT 15)

if (MSVC)
  target_compile_options(vix_http_p1_protocol_test PRIVATE /UNDEBUG)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
  target_compile_options(vix_http_p1_protocol_test PRIVATE -UNDEBUG)
endif()

if (VIX_BUILD_HTTP_EXPERIMENTAL)
  add_executable(vix_http_public_api_test public_api_test.cpp)
  target_compile_features(vix_http_public_api_test PRIVATE cxx_std_20)
  target_link_libraries(vix_http_public_api_test PRIVATE vix::http_experimental)
  add_test(NAME vix_http_public_api_test COMMAND vix_http_public_api_test)
  set_tests_properties(vix_http_public_api_test PROPERTIES TIMEOUT 30)
  if (MSVC)
    target_compile_options(vix_http_public_api_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(vix_http_public_api_test PRIVATE -UNDEBUG)
  endif()
endif()

if (VIX_BUILD_HTTP_TLS_EXPERIMENTAL)
  find_package(Threads REQUIRED)
  add_executable(vix_http_https_public_api_test https_public_api_test.cpp)
  target_compile_features(vix_http_https_public_api_test PRIVATE cxx_std_20)
  target_link_libraries(vix_http_https_public_api_test PRIVATE
    vix::http_tls_experimental Threads::Threads)
  add_test(NAME vix_http_https_public_api_test
    COMMAND vix_http_https_public_api_test)
  set_tests_properties(vix_http_https_public_api_test PROPERTIES TIMEOUT 45)
  if (MSVC)
    target_compile_options(vix_http_https_public_api_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(vix_http_https_public_api_test PRIVATE -UNDEBUG)
  endif()
endif()

# Private H0-P2 composition proof. It is still gated by the opt-in HTTP test
# option and is not installed or exported with the SDK.
if (VIX_HAS_NETWORK AND NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
  find_package(Threads REQUIRED)
  add_executable(vix_http_private_exchange_test
    private_exchange_test.cpp
    ../src/Http1.cpp
    ../src/PrivateExchange.cpp)
  target_compile_features(vix_http_private_exchange_test PRIVATE cxx_std_20)
  target_compile_definitions(vix_http_private_exchange_test PRIVATE
    VIX_NETWORK_TEST_CAPTURE=1)
  target_include_directories(vix_http_private_exchange_test PRIVATE
    ../src
    ../../network/src
    ../../network/tests)
  target_link_libraries(vix_http_private_exchange_test PRIVATE
    vix_network vix::standard Threads::Threads)
  add_test(NAME vix_http_private_exchange_test COMMAND vix_http_private_exchange_test)
  set_tests_properties(vix_http_private_exchange_test PROPERTIES TIMEOUT 30)
  if (MSVC)
    target_compile_options(vix_http_private_exchange_test PRIVATE /UNDEBUG)
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(vix_http_private_exchange_test PRIVATE -UNDEBUG)
  endif()
endif()

# Private HTTPS composition proof. It is opt-in with both the private HTTP
# tests and explicitly experimental TLS; it is never installed or exported.
if (VIX_HAS_NETWORK AND TARGET vix_tls_experimental AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
  find_package(Threads REQUIRED)
  add_executable(vix_http_private_tls_exchange_test
    private_tls_exchange_test.cpp
    ../src/Http1.cpp
    ../src/PrivateTlsExchange.cpp)
  target_compile_features(vix_http_private_tls_exchange_test PRIVATE cxx_std_20)
  target_compile_definitions(vix_http_private_tls_exchange_test PRIVATE
    VIX_NETWORK_TEST_CAPTURE=1)
  target_include_directories(vix_http_private_tls_exchange_test PRIVATE
    ../src
    ../../tls/src
    ../../network/src
    ../../network/tests)
  target_link_libraries(vix_http_private_tls_exchange_test PRIVATE
    vix_tls_experimental vix_network vix::standard Threads::Threads)
  add_test(NAME vix_http_private_tls_exchange_test
    COMMAND vix_http_private_tls_exchange_test)
  set_tests_properties(vix_http_private_tls_exchange_test PROPERTIES TIMEOUT 60)
  if (MSVC)
    target_compile_options(vix_http_private_tls_exchange_test PRIVATE /UNDEBUG)
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(vix_http_private_tls_exchange_test PRIVATE -UNDEBUG)
  endif()
endif()
