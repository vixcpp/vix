# Assertions must remain active in test binaries, including Release.
# Production libraries retain their normal NDEBUG configuration.
if(MSVC)
  add_compile_options(/UNDEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-UNDEBUG)
endif()

find_package(Threads REQUIRED)
add_executable(vix_tls_i0b_private_driver_test
  private_driver_test.cpp
)
target_compile_features(vix_tls_i0b_private_driver_test PRIVATE cxx_std_20)
target_include_directories(vix_tls_i0b_private_driver_test PRIVATE
  ../src
  ../../network/src
)
find_package(Threads REQUIRED)
target_link_libraries(vix_tls_i0b_private_driver_test PRIVATE
  vix::tls_experimental
  vix_network
  vix::standard
  Threads::Threads
)
add_test(NAME vix_tls_i0b_private_driver_test COMMAND vix_tls_i0b_private_driver_test)
set_tests_properties(vix_tls_i0b_private_driver_test PROPERTIES TIMEOUT 60)
