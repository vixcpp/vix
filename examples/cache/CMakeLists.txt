cmake_minimum_required(VERSION 3.20)

project(vix_example_cache LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_cache_examples
  01_memory_cache_basic
  02_file_cache_persistence
  03_lru_cache_eviction
  04_offline_and_network_error
  05_cache_key_and_headers
)

enable_testing()

foreach (_example IN LISTS _vix_cache_examples)
  add_executable("cache_${_example}" "${_example}.cpp")
  target_compile_features("cache_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("cache_${_example}" PRIVATE vix::cache)

  set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime/${_example}")
  file(MAKE_DIRECTORY "${_runtime_dir}")
  add_test(NAME "cache.${_example}" COMMAND "cache_${_example}")
  set_tests_properties("cache.${_example}" PROPERTIES WORKING_DIRECTORY "${_runtime_dir}")
endforeach()
