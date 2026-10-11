if (NOT DEFINED VIX_BINARY_DIR OR NOT DEFINED VIX_SOURCE_DIR)
  message(FATAL_ERROR "VIX_BINARY_DIR and VIX_SOURCE_DIR are required")
endif()

set(_prefix "${VIX_BINARY_DIR}/sdk-package-prefix")
set(_consumer_build "${VIX_BINARY_DIR}/sdk-package-consumer-build")
file(REMOVE_RECURSE "${_prefix}" "${_consumer_build}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${VIX_BINARY_DIR}" --prefix "${_prefix}"
  RESULT_VARIABLE _install_result
)
if (NOT _install_result EQUAL 0)
  message(FATAL_ERROR "Vix SDK install failed")
endif()
file(READ "${_prefix}/lib/cmake/Vix/VixTargets.cmake" _targets)
foreach(_forbidden "${VIX_SOURCE_DIR}" "${VIX_BINARY_DIR}" "domains/network/src")
  string(FIND "${_targets}" "${_forbidden}" _forbidden_at)
  if (NOT _forbidden_at EQUAL -1)
    message(FATAL_ERROR "Installed Vix targets leak: ${_forbidden}")
  endif()
endforeach()
file(GLOB_RECURSE _private_headers
  "${_prefix}/include/*Engine.hpp"
  "${_prefix}/include/*CoroutineTask.hpp"
  "${_prefix}/include/*phase*_test.hpp")
if (_private_headers)
  message(FATAL_ERROR "Installed Vix SDK contains private test headers: ${_private_headers}")
endif()
file(GLOB _files_private_headers "${_prefix}/include/vix/files/detail/*")
if (_files_private_headers)
  message(FATAL_ERROR "Installed Vix SDK contains Files private headers: ${_files_private_headers}")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_consumer" -B "${_consumer_build}"
    "-DCMAKE_PREFIX_PATH=${_prefix}"
  RESULT_VARIABLE _configure_result
)
if (NOT _configure_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer configuration failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_consumer_build}"
  RESULT_VARIABLE _build_result
)
if (NOT _build_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer build failed")
endif()
execute_process(
  COMMAND "${_consumer_build}/vix_sdk_consumer"
  RESULT_VARIABLE _runtime_result
)
if (NOT _runtime_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer runtime failed")
endif()
