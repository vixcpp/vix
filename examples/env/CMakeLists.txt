cmake_minimum_required(VERSION 3.20)

project(vix_example_env LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_env_examples
  basic
  get_bool
  layered_env
  load_env_file
  load_layered_env
  production_env
)

set(_vix_env_runnable_examples
  basic
  get_bool
  layered_env
  load_layered_env
  production_env
)

enable_testing()

foreach (_example IN LISTS _vix_env_examples)
  add_executable("env_${_example}" "${_example}.cpp")
  target_compile_features("env_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("env_${_example}" PRIVATE vix::env)
endforeach()

foreach (_example IN LISTS _vix_env_runnable_examples)
  add_test(NAME "env.${_example}" COMMAND "env_${_example}")
endforeach()

set_tests_properties(env.basic PROPERTIES
  ENVIRONMENT "APP_NAME=Vix CI;APP_ENV=test"
)
set_tests_properties(env.get_bool PROPERTIES ENVIRONMENT "APP_DEBUG=true")
