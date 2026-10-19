if (VIX_BUILD_HTTP_EXPERIMENTAL)
  add_library(vix_http_experimental STATIC
    src/Http1.cpp
    src/PrivateExchange.cpp
    src/HttpApi.cpp)
  add_library(vix::http_experimental ALIAS vix_http_experimental)
  target_compile_features(vix_http_experimental PUBLIC cxx_std_20)
  target_include_directories(vix_http_experimental
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/domains/network/src)
  target_link_libraries(vix_http_experimental PUBLIC vix::network vix::standard)
  set_target_properties(vix_http_experimental PROPERTIES EXPORT_NAME http_experimental)
  add_executable(vix_http_client_example EXCLUDE_FROM_ALL examples/client.cpp)
  target_compile_features(vix_http_client_example PRIVATE cxx_std_20)
  target_link_libraries(vix_http_client_example PRIVATE vix::http_experimental)
  add_executable(vix_http_server_example EXCLUDE_FROM_ALL examples/server.cpp)
  target_compile_features(vix_http_server_example PRIVATE cxx_std_20)
  target_link_libraries(vix_http_server_example PRIVATE vix::http_experimental)
  if (VIX_INSTALL_HTTP_EXPERIMENTAL)
    install(TARGETS vix_http_experimental EXPORT VixHttpTargets
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(FILES include/vix/http/Http.hpp
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vix/http")
  endif()
endif()

if (VIX_BUILD_HTTP_TLS_EXPERIMENTAL)
  add_library(vix_http_tls_experimental STATIC
    src/HttpsApi.cpp
    src/PrivateTlsExchange.cpp)
  add_library(vix::http_tls_experimental ALIAS vix_http_tls_experimental)
  target_compile_features(vix_http_tls_experimental PUBLIC cxx_std_20)
  target_include_directories(vix_http_tls_experimental
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/domains/network/src
      ${CMAKE_SOURCE_DIR}/domains/tls/src)
  target_link_libraries(vix_http_tls_experimental
    PUBLIC vix::http_experimental vix::tls_experimental)
  set_target_properties(vix_http_tls_experimental PROPERTIES
    EXPORT_NAME http_tls_experimental)
  add_executable(vix_https_client_example EXCLUDE_FROM_ALL examples/https_client.cpp)
  target_compile_features(vix_https_client_example PRIVATE cxx_std_20)
  target_link_libraries(vix_https_client_example PRIVATE vix::http_tls_experimental)
  add_executable(vix_https_server_example EXCLUDE_FROM_ALL examples/https_server.cpp)
  target_compile_features(vix_https_server_example PRIVATE cxx_std_20)
  target_link_libraries(vix_https_server_example PRIVATE vix::http_tls_experimental)
  if (VIX_INSTALL_HTTP_TLS_EXPERIMENTAL)
    install(TARGETS vix_http_tls_experimental EXPORT VixHttpTlsTargets
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(FILES include/vix/http/Https.hpp
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vix/http")
  endif()
endif()

if (VIX_BUILD_HTTP_P1_TESTS)
  add_subdirectory(tests)
endif()
