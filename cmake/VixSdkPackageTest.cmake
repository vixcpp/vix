if (NOT DEFINED VIX_BINARY_DIR OR NOT DEFINED VIX_SOURCE_DIR)
  message(FATAL_ERROR "VIX_BINARY_DIR and VIX_SOURCE_DIR are required")
endif()
if (NOT DEFINED VIX_EXPECT_NETWORK)
  message(FATAL_ERROR "VIX_EXPECT_NETWORK is required")
endif()
if (NOT DEFINED VIX_TARGET_SYSTEM)
  message(FATAL_ERROR "VIX_TARGET_SYSTEM is required")
endif()
if (VIX_TARGET_SYSTEM STREQUAL "WASI" AND VIX_EXPECT_NETWORK AND
    (NOT DEFINED VIX_WASMTIME_EXECUTABLE OR NOT EXISTS "${VIX_WASMTIME_EXECUTABLE}"))
  message(FATAL_ERROR "The WASI installed-SDK consumer requires VIX_WASMTIME_EXECUTABLE.")
endif()
set(_consumer_cmake_args)
set(_consumer_generator_args)
set(_configuration_args)
if (DEFINED VIX_CONFIGURATION AND NOT VIX_CONFIGURATION STREQUAL "")
  list(APPEND _configuration_args --config "${VIX_CONFIGURATION}")
  list(APPEND _consumer_cmake_args "-DCMAKE_BUILD_TYPE=${VIX_CONFIGURATION}")
endif()
if (DEFINED VIX_GENERATOR AND NOT VIX_GENERATOR STREQUAL "")
  list(APPEND _consumer_generator_args -G "${VIX_GENERATOR}")
endif()
if (DEFINED VIX_GENERATOR_PLATFORM AND NOT VIX_GENERATOR_PLATFORM STREQUAL "")
  list(APPEND _consumer_generator_args -A "${VIX_GENERATOR_PLATFORM}")
endif()
if (DEFINED VIX_GENERATOR_TOOLSET AND NOT VIX_GENERATOR_TOOLSET STREQUAL "")
  list(APPEND _consumer_generator_args -T "${VIX_GENERATOR_TOOLSET}")
endif()
if (DEFINED VIX_SANITIZER_LINK_FLAGS AND NOT VIX_SANITIZER_LINK_FLAGS STREQUAL "")
  # The installed archives in this test came from an instrumented build. Link
  # only this isolated test consumer with the matching runtime; exported SDK
  # targets remain free of sanitizer usage requirements for ordinary users.
  list(APPEND _consumer_cmake_args
    "-DCMAKE_EXE_LINKER_FLAGS=${VIX_SANITIZER_LINK_FLAGS}")
endif()
if (DEFINED VIX_TOOLCHAIN_FILE AND NOT VIX_TOOLCHAIN_FILE STREQUAL "")
  list(APPEND _consumer_cmake_args "-DCMAKE_TOOLCHAIN_FILE=${VIX_TOOLCHAIN_FILE}")
endif()
if (DEFINED VIX_WASI_SDK_ROOT AND NOT VIX_WASI_SDK_ROOT STREQUAL "")
  list(APPEND _consumer_cmake_args "-DVIX_WASI_SDK_ROOT=${VIX_WASI_SDK_ROOT}")
endif()
if (DEFINED VIX_OSX_ARCHITECTURES AND NOT VIX_OSX_ARCHITECTURES STREQUAL "")
  list(APPEND _consumer_cmake_args "-DCMAKE_OSX_ARCHITECTURES=${VIX_OSX_ARCHITECTURES}")
endif()

set(_prefix "${VIX_BINARY_DIR}/sdk-package-prefix")
set(_relocated_prefix "${VIX_BINARY_DIR}/sdk-package-relocated-prefix")
set(_consumer_build "${VIX_BINARY_DIR}/sdk-package-consumer-build")
file(REMOVE_RECURSE "${_prefix}" "${_relocated_prefix}" "${_consumer_build}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${VIX_BINARY_DIR}" --prefix "${_prefix}"
    ${_configuration_args}
  RESULT_VARIABLE _install_result
)
if (NOT _install_result EQUAL 0)
  message(FATAL_ERROR "Vix SDK install failed")
endif()
file(READ "${_prefix}/lib/cmake/Vix/VixTargets.cmake" _targets)
foreach(_forbidden
    "${VIX_SOURCE_DIR}"
    "${VIX_BINARY_DIR}"
    "${VIX_SOURCE_DIR}/domains/files/src"
    "${VIX_SOURCE_DIR}/domains/process/src"
    "ProcessBridge.hpp"
    "NativePath.hpp")
  string(FIND "${_targets}" "${_forbidden}" _forbidden_at)
  if (NOT _forbidden_at EQUAL -1)
    message(FATAL_ERROR "Installed Vix targets leak: ${_forbidden}")
  endif()
endforeach()

set(_required_targets
    "vix::vix_standard"
    "vix::vix_concurrency"
    "vix::vix_files"
    "vix::vix_process")
if (VIX_EXPECT_NETWORK)
  list(APPEND _required_targets "vix::vix_network")
endif()
foreach(_required_target IN LISTS _required_targets)
  string(FIND "${_targets}" "${_required_target}" _target_at)
  if (_target_at EQUAL -1)
    message(FATAL_ERROR "Installed Vix SDK is missing canonical target: ${_required_target}")
  endif()
endforeach()
if (NOT VIX_EXPECT_NETWORK)
  string(FIND "${_targets}" "vix::vix_network" _unexpected_network_target)
  if (NOT _unexpected_network_target EQUAL -1)
    message(FATAL_ERROR "Network-disabled Vix SDK unexpectedly exports vix::vix_network")
  endif()
endif()

foreach(_forbidden_target
    "vix::vix_fs"
    "vix::vix_db"
    "vix::vix_orm"
    "vix::vix_p2p"
    "vix::vix_websocket"
    "vix::process"
    "vix::db"
    "vix::orm"
    "vix::p2p"
    "vix::websocket")
  string(FIND "${_targets}" "${_forbidden_target}" _target_at)
  if (NOT _target_at EQUAL -1)
    message(FATAL_ERROR "Installed Vix SDK exposes historical target: ${_forbidden_target}")
  endif()
endforeach()

foreach(_forbidden_header_dir fs db orm p2p websocket)
  if (EXISTS "${_prefix}/include/vix/${_forbidden_header_dir}")
    message(FATAL_ERROR "Installed Vix SDK exposes historical headers: vix/${_forbidden_header_dir}")
  endif()
endforeach()
if (EXISTS "${_prefix}/include/vix/process/Command.hpp")
  message(FATAL_ERROR "Installed Vix SDK contains the historical Process command API")
endif()

# The removed architecture/profile switches must not restore the historical
# root install rules. Configure a fresh tree with their old escape-hatch values
# and inspect generated install scripts without building or installing it.
set(_obsolete_flag_build "${VIX_BINARY_DIR}/sdk-obsolete-flag-build")
file(REMOVE_RECURSE "${_obsolete_flag_build}")
execute_process(
  COMMAND "${CMAKE_COMMAND}"
    -S "${VIX_SOURCE_DIR}"
    -B "${_obsolete_flag_build}"
    -DVIX_BUILD_CANONICAL_ARCHITECTURE=OFF
    -DVIX_SDK_PROFILE=all
    -DVIX_BUILD_TESTS=OFF
    -DVIX_BUILD_EXAMPLES=OFF
    "-DVIX_BUILD_NETWORK=${VIX_EXPECT_NETWORK}"
    ${_consumer_cmake_args}
  RESULT_VARIABLE _obsolete_flag_configure_result
)
if (NOT _obsolete_flag_configure_result EQUAL 0)
  message(FATAL_ERROR "Vix configuration with obsolete SDK flags failed")
endif()
file(GLOB_RECURSE _obsolete_flag_install_scripts
  "${_obsolete_flag_build}/cmake_install.cmake")
if (NOT _obsolete_flag_install_scripts)
  message(FATAL_ERROR "Obsolete-flag configuration produced no install scripts")
endif()
foreach(_install_script IN LISTS _obsolete_flag_install_scripts)
  file(READ "${_install_script}" _install_contents)
  string(FIND "${_install_contents}" "${VIX_SOURCE_DIR}/modules/" _historical_install_at)
  if (NOT _historical_install_at EQUAL -1)
    message(FATAL_ERROR "Obsolete SDK flags restored historical module installation")
  endif()
endforeach()

file(GLOB_RECURSE _private_headers
  "${_prefix}/include/*Engine.hpp"
  "${_prefix}/include/*CoroutineTask.hpp"
  "${_prefix}/include/*phase*_test.hpp")
if (_private_headers)
  message(FATAL_ERROR "Installed Vix SDK contains private test headers: ${_private_headers}")
endif()
foreach(_private_header_dir files/detail process/detail network/detail)
  file(GLOB _private_headers "${_prefix}/include/vix/${_private_header_dir}/*")
  if (_private_headers)
    message(FATAL_ERROR "Installed Vix SDK contains implementation-only headers under vix/${_private_header_dir}: ${_private_headers}")
  endif()
endforeach()
foreach(_private_header
    "${_prefix}/include/vix/files/detail/ProcessBridge.hpp"
    "${_prefix}/include/vix/files/detail/NativePath.hpp")
  if (EXISTS "${_private_header}")
    message(FATAL_ERROR "Installed Vix SDK exposes private Files bridge header: ${_private_header}")
  endif()
endforeach()

file(RENAME "${_prefix}" "${_relocated_prefix}")
if (VIX_EXPECT_NETWORK)
  set(_consumer_source "${VIX_SOURCE_DIR}/tests/sdk_consumer")
else()
  set(_consumer_source "${VIX_SOURCE_DIR}/tests/sdk_consumer_no_network")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${_consumer_source}" -B "${_consumer_build}"
    "-DCMAKE_PREFIX_PATH=${_relocated_prefix}"
    ${_consumer_generator_args}
    ${_consumer_cmake_args}
  RESULT_VARIABLE _configure_result
)
if (NOT _configure_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer configuration failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_consumer_build}" --parallel 2
    ${_configuration_args}
  RESULT_VARIABLE _build_result
)
if (NOT _build_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer build failed")
endif()
if (VIX_EXPECT_NETWORK)
  set(_consumer_path_file "${_consumer_build}/vix_sdk_consumer_path-${VIX_CONFIGURATION}.txt")
  if (NOT EXISTS "${_consumer_path_file}")
    message(FATAL_ERROR "External SDK consumer did not generate its configuration-specific target path: ${_consumer_path_file}")
  endif()
  file(READ "${_consumer_path_file}" _consumer_executable)
  string(STRIP "${_consumer_executable}" _consumer_executable)
  if (_consumer_executable STREQUAL "" OR NOT EXISTS "${_consumer_executable}")
    message(FATAL_ERROR "External SDK consumer executable is missing for configuration '${VIX_CONFIGURATION}': ${_consumer_executable}")
  endif()
  if (VIX_TARGET_SYSTEM STREQUAL "WASI")
    execute_process(
      COMMAND "${VIX_WASMTIME_EXECUTABLE}" run "${_consumer_executable}"
      RESULT_VARIABLE _runtime_result
    )
  else()
    execute_process(
      COMMAND "${_consumer_executable}"
      RESULT_VARIABLE _runtime_result
    )
  endif()
  if (NOT _runtime_result EQUAL 0)
    message(FATAL_ERROR "External Vix SDK consumer runtime failed")
  endif()
else()
  if (NOT VIX_TARGET_SYSTEM STREQUAL "WASI")
    execute_process(
      COMMAND "${_consumer_build}/vix_sdk_files_process_consumer"
      RESULT_VARIABLE _runtime_result
    )
    if (NOT _runtime_result EQUAL 0)
      message(FATAL_ERROR "External Files/Process-only Vix SDK consumer runtime failed")
    endif()
  endif()
  set(_network_required_build "${VIX_BINARY_DIR}/sdk-network-required-build")
  file(REMOVE_RECURSE "${_network_required_build}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_consumer_network_required"
      -B "${_network_required_build}" "-DCMAKE_PREFIX_PATH=${_relocated_prefix}"
      ${_consumer_cmake_args}
    RESULT_VARIABLE _network_required_result
    OUTPUT_VARIABLE _network_required_stdout
    ERROR_VARIABLE _network_required_stderr
  )
  if (_network_required_result EQUAL 0)
    message(FATAL_ERROR "Network-disabled Vix SDK accepted a required Network component")
  endif()
  string(FIND "${_network_required_stdout}${_network_required_stderr}" "Network" _network_error_at)
  if (_network_error_at EQUAL -1)
    message(FATAL_ERROR "Unavailable Network component failure was not explicit")
  endif()
endif()
