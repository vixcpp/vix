foreach(_required VIX_BINARY_DIR VIX_SOURCE_DIR)
  if (NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required")
  endif()
endforeach()

string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef _run_id)
set(_root "${VIX_BINARY_DIR}/tls-pkg1-${_run_id}")
set(_prefix "${_root}/prefix")
set(_relocated "${_root}/relocated")
set(_tls_build "${_root}/tls-consumer-build")
set(_disabled_build "${_root}/disabled-consumer-build")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${VIX_BINARY_DIR}" --prefix "${_prefix}"
  RESULT_VARIABLE _install_result
)
if (NOT _install_result EQUAL 0)
  message(FATAL_ERROR "Experimental TLS SDK installation failed")
endif()

set(_config_dir "${_prefix}/lib/cmake/Vix")
foreach(_expected
    "${_config_dir}/VixConfig.cmake"
    "${_config_dir}/VixTlsTargets.cmake"
    "${_prefix}/include/vix/tls/Errors.hpp"
    "${_prefix}/include/vix/tls/TlsConfig.hpp"
    "${_prefix}/include/vix/tls/TlsChannel.hpp")
  if (NOT EXISTS "${_expected}")
    message(FATAL_ERROR "TLS SDK installation is missing: ${_expected}")
  endif()
endforeach()
foreach(_private
    "${_prefix}/include/vix/tls/OpenSslDriver.hpp"
    "${_prefix}/include/vix/tls/PrivateChannel.hpp"
    "${_prefix}/include/vix/network/Engine.hpp")
  if (EXISTS "${_private}")
    message(FATAL_ERROR "TLS SDK installation leaked a private header: ${_private}")
  endif()
endforeach()

file(GLOB _tls_target_files "${_config_dir}/VixTlsTargets*.cmake")
if (NOT _tls_target_files)
  message(FATAL_ERROR "TLS export metadata is missing")
endif()
foreach(_forbidden "${VIX_SOURCE_DIR}" "${VIX_BINARY_DIR}/domains")
  foreach(_tls_target_file IN LISTS _tls_target_files)
    file(READ "${_tls_target_file}" _tls_targets)
    string(FIND "${_tls_targets}" "${_forbidden}" _leak_at)
    if (NOT _leak_at EQUAL -1)
      message(FATAL_ERROR "Installed TLS target leaks a repository path: ${_forbidden}")
    endif()
  endforeach()
endforeach()

file(RENAME "${_prefix}" "${_relocated}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_tls_consumer"
    -B "${_tls_build}" "-DCMAKE_PREFIX_PATH=${_relocated}"
    -DCMAKE_BUILD_TYPE=Debug
  RESULT_VARIABLE _tls_configure
)
if (NOT _tls_configure EQUAL 0)
  message(FATAL_ERROR "Relocated TLS consumer configuration failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_tls_build}" --parallel 2
  RESULT_VARIABLE _tls_build_result
)
if (NOT _tls_build_result EQUAL 0)
  message(FATAL_ERROR "Relocated TLS consumer build failed")
endif()
execute_process(
  COMMAND "${_tls_build}/vix_tls_package_consumer"
  RESULT_VARIABLE _tls_run
)
if (NOT _tls_run EQUAL 0)
  message(FATAL_ERROR "Relocated TLS consumer runtime failed")
endif()

# A consumer which does not request the optional component must not trigger
# OpenSSL discovery, even when the installed prefix also contains TLS.
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_tls_disabled_consumer"
    -B "${_disabled_build}" "-DCMAKE_PREFIX_PATH=${_relocated}"
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE
  RESULT_VARIABLE _disabled_configure
)
if (NOT _disabled_configure EQUAL 0)
  message(FATAL_ERROR "TLS-disabled consumer unexpectedly required OpenSSL")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_disabled_build}" --parallel 2
  RESULT_VARIABLE _disabled_build_result
)
if (NOT _disabled_build_result EQUAL 0)
  message(FATAL_ERROR "TLS-disabled consumer build failed")
endif()
execute_process(
  COMMAND "${_disabled_build}/vix_tls_disabled_package_consumer"
  RESULT_VARIABLE _disabled_run
)
if (NOT _disabled_run EQUAL 0)
  message(FATAL_ERROR "TLS-disabled consumer runtime failed")
endif()

# Requesting the TLS component with OpenSSL discovery disabled must fail
# clearly rather than yielding a partially usable TLS target.
set(_missing_openssl_build "${_root}/missing-openssl-build")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${VIX_SOURCE_DIR}/tests/sdk_tls_consumer"
    -B "${_missing_openssl_build}" "-DCMAKE_PREFIX_PATH=${_relocated}"
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE
  RESULT_VARIABLE _missing_openssl_result
  OUTPUT_VARIABLE _missing_openssl_stdout
  ERROR_VARIABLE _missing_openssl_stderr
)
if (_missing_openssl_result EQUAL 0)
  message(FATAL_ERROR "TLS component unexpectedly configured without OpenSSL")
endif()
string(FIND "${_missing_openssl_stdout}${_missing_openssl_stderr}"
  "OpenSSL" _openssl_error_at)
if (_openssl_error_at EQUAL -1)
  message(FATAL_ERROR "Missing TLS OpenSSL dependency failure was not explicit")
endif()

file(REMOVE_RECURSE "${_root}")
