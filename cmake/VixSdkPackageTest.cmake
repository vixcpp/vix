if (NOT DEFINED VIX_BINARY_DIR OR NOT DEFINED VIX_SOURCE_DIR)
  message(FATAL_ERROR "VIX_BINARY_DIR and VIX_SOURCE_DIR are required")
endif()

set(_prefix "${VIX_BINARY_DIR}/sdk-package-prefix")
set(_relocated_prefix "${VIX_BINARY_DIR}/sdk-package-relocated-prefix")
set(_consumer_build "${VIX_BINARY_DIR}/sdk-package-consumer-build")
file(REMOVE_RECURSE "${_prefix}" "${_relocated_prefix}" "${_consumer_build}")
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

foreach(_required_target
    "vix::vix_standard"
    "vix::vix_files"
    "vix::vix_process"
    "vix::vix_network")
  string(FIND "${_targets}" "${_required_target}" _target_at)
  if (_target_at EQUAL -1)
    message(FATAL_ERROR "Installed Vix SDK is missing canonical target: ${_required_target}")
  endif()
endforeach()

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
file(GLOB _files_private_headers "${_prefix}/include/vix/files/detail/*")
if (_files_private_headers)
  message(FATAL_ERROR "Installed Vix SDK contains Files private headers: ${_files_private_headers}")
endif()

file(RENAME "${_prefix}" "${_relocated_prefix}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_consumer" -B "${_consumer_build}"
    "-DCMAKE_PREFIX_PATH=${_relocated_prefix}"
  RESULT_VARIABLE _configure_result
)
if (NOT _configure_result EQUAL 0)
  message(FATAL_ERROR "External Vix SDK consumer configuration failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_consumer_build}" --parallel 2
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
