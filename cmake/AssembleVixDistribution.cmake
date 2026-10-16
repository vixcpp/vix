# Assemble only outputs admitted by the current Vix distribution contract.
# Invoke with -DVIX_CLI_BINARY=... -DVIX_SDK_PREFIX=... -DVIX_SDK_LIBRARIES=...
# -DVIX_CLI_RUNTIME_LIBRARIES=... -DVIX_INSTALLER_RUNTIME_LIBRARIES=...
# -DVIX_RELEASE_TRUST=...
# -DVIX_DISTRIBUTION_SOURCE=... -DVIX_OUTPUT_ROOT=...
foreach(required VIX_CLI_BINARY VIX_INSTALLER_BINARY VIX_SDK_PREFIX VIX_SDK_LIBRARIES VIX_RELEASE_TRUST
    VIX_DISTRIBUTION_SOURCE VIX_OUTPUT_ROOT VIX_BOOTSTRAP_OUTPUT_ROOT)
  if (NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required for Vix distribution assembly")
  endif()
endforeach()
foreach(runtime_list VIX_CLI_RUNTIME_LIBRARIES VIX_INSTALLER_RUNTIME_LIBRARIES VIX_RUNTIME_LICENSE_FILES)
  if (NOT DEFINED ${runtime_list})
    message(FATAL_ERROR "${runtime_list} must be explicitly provided (an empty list is allowed)")
  endif()
endforeach()

if (NOT EXISTS "${VIX_CLI_BINARY}" OR IS_DIRECTORY "${VIX_CLI_BINARY}")
  message(FATAL_ERROR "VIX_CLI_BINARY must name the built current-Vix executable")
endif()
if (NOT EXISTS "${VIX_INSTALLER_BINARY}" OR IS_DIRECTORY "${VIX_INSTALLER_BINARY}")
  message(FATAL_ERROR "VIX_INSTALLER_BINARY must name the built vix-installer executable")
endif()
if (NOT IS_DIRECTORY "${VIX_SDK_PREFIX}/include" OR NOT IS_DIRECTORY "${VIX_SDK_PREFIX}/lib/cmake/Vix")
  message(FATAL_ERROR "VIX_SDK_PREFIX must be an installed canonical Vix SDK prefix")
endif()
foreach(sdk_config VixConfig.cmake VixConfigVersion.cmake VixTargets.cmake)
  if (NOT EXISTS "${VIX_SDK_PREFIX}/lib/cmake/Vix/${sdk_config}")
    message(FATAL_ERROR "Canonical SDK package file is missing: ${sdk_config}")
  endif()
endforeach()
if (NOT EXISTS "${VIX_RELEASE_TRUST}" OR NOT EXISTS "${VIX_DISTRIBUTION_SOURCE}")
  message(FATAL_ERROR "release public trust store and distribution source config are required")
endif()
file(SIZE "${VIX_RELEASE_TRUST}" _vix_trust_size)
file(SIZE "${VIX_DISTRIBUTION_SOURCE}" _vix_source_size)
if (_vix_trust_size EQUAL 0 OR _vix_source_size EQUAL 0)
  message(FATAL_ERROR "release trust and distribution source provisioning files must not be empty")
endif()

if (EXISTS "${VIX_OUTPUT_ROOT}")
  message(FATAL_ERROR "VIX_OUTPUT_ROOT must not exist; refusing to mix release inputs with stale output")
endif()
if (EXISTS "${VIX_BOOTSTRAP_OUTPUT_ROOT}")
  message(FATAL_ERROR "VIX_BOOTSTRAP_OUTPUT_ROOT must not exist; refusing to mix bootstrap inputs with stale output")
endif()

file(MAKE_DIRECTORY
  "${VIX_OUTPUT_ROOT}/bin"
  "${VIX_OUTPUT_ROOT}/include"
  "${VIX_OUTPUT_ROOT}/lib"
  "${VIX_OUTPUT_ROOT}/share/vix/trust"
  "${VIX_OUTPUT_ROOT}/share/vix/distribution"
  "${VIX_OUTPUT_ROOT}/share/completions/bash"
  "${VIX_OUTPUT_ROOT}/share/completions/zsh"
  "${VIX_OUTPUT_ROOT}/share/completions/fish")
configure_file("${VIX_CLI_BINARY}" "${VIX_OUTPUT_ROOT}/bin/vix" COPYONLY)
file(CHMOD "${VIX_OUTPUT_ROOT}/bin/vix"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(COPY "${VIX_SDK_PREFIX}/include/" DESTINATION "${VIX_OUTPUT_ROOT}/include")
file(COPY "${VIX_SDK_PREFIX}/lib/cmake/Vix/" DESTINATION "${VIX_OUTPUT_ROOT}/lib/cmake/Vix")
foreach(sdk_library IN LISTS VIX_SDK_LIBRARIES)
  if (NOT EXISTS "${sdk_library}" OR IS_DIRECTORY "${sdk_library}")
    message(FATAL_ERROR "A listed canonical SDK library does not exist: ${sdk_library}")
  endif()
  get_filename_component(sdk_library_name "${sdk_library}" NAME)
  configure_file("${sdk_library}" "${VIX_OUTPUT_ROOT}/lib/${sdk_library_name}" COPYONLY)
endforeach()
# Only explicitly resolved runtime dependencies belong here; never sweep build
# or repository module directories into the distribution by globbing.
function(vix_copy_runtime_libraries libraries destination label)
  foreach(runtime_library IN LISTS ${libraries})
    if (NOT EXISTS "${runtime_library}" OR IS_DIRECTORY "${runtime_library}")
      message(FATAL_ERROR "A listed ${label} runtime dependency does not exist: ${runtime_library}")
    endif()
    get_filename_component(runtime_library_name "${runtime_library}" NAME)
    if (EXISTS "${destination}/lib/${runtime_library_name}")
      message(FATAL_ERROR "${label} runtime dependency collides with a packaged library: ${runtime_library_name}")
    endif()
    configure_file("${runtime_library}" "${destination}/lib/${runtime_library_name}" COPYONLY)
  endforeach()
endfunction()

function(vix_copy_runtime_license_files destination)
  file(MAKE_DIRECTORY "${destination}/share/licenses/runtime")
  foreach(license_file IN LISTS VIX_RUNTIME_LICENSE_FILES)
    if (NOT EXISTS "${license_file}" OR IS_DIRECTORY "${license_file}")
      message(FATAL_ERROR "A listed runtime license notice does not exist: ${license_file}")
    endif()
    get_filename_component(license_name "${license_file}" NAME)
    if (EXISTS "${destination}/share/licenses/runtime/${license_name}")
      message(FATAL_ERROR "Runtime license notice basename collision: ${license_name}")
    endif()
    configure_file("${license_file}" "${destination}/share/licenses/runtime/${license_name}" COPYONLY)
  endforeach()
endfunction()

vix_copy_runtime_libraries(VIX_CLI_RUNTIME_LIBRARIES "${VIX_OUTPUT_ROOT}" "CLI")
vix_copy_runtime_license_files("${VIX_OUTPUT_ROOT}")
configure_file("${VIX_RELEASE_TRUST}" "${VIX_OUTPUT_ROOT}/share/vix/trust/releases.toml" COPYONLY)
configure_file("${VIX_DISTRIBUTION_SOURCE}" "${VIX_OUTPUT_ROOT}/share/vix/distribution/source.toml" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/share/completions/bash/vix"
               "${VIX_OUTPUT_ROOT}/share/completions/bash/vix" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/share/completions/zsh/_vix"
               "${VIX_OUTPUT_ROOT}/share/completions/zsh/_vix" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/share/completions/fish/vix.fish"
               "${VIX_OUTPUT_ROOT}/share/completions/fish/vix.fish" COPYONLY)
file(MAKE_DIRECTORY "${VIX_OUTPUT_ROOT}/share/doc/vix")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/README.md"
               "${VIX_OUTPUT_ROOT}/share/doc/vix/README.md" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../LICENSE"
               "${VIX_OUTPUT_ROOT}/share/doc/vix/LICENSE" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/LICENSE"
               "${VIX_OUTPUT_ROOT}/share/doc/vix/CLI-LICENSE" COPYONLY)

file(MAKE_DIRECTORY
  "${VIX_BOOTSTRAP_OUTPUT_ROOT}/bin"
  "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/vix/trust"
  "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/vix/distribution")
configure_file("${VIX_INSTALLER_BINARY}" "${VIX_BOOTSTRAP_OUTPUT_ROOT}/bin/vix-installer" COPYONLY)
file(CHMOD "${VIX_BOOTSTRAP_OUTPUT_ROOT}/bin/vix-installer"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(MAKE_DIRECTORY "${VIX_BOOTSTRAP_OUTPUT_ROOT}/lib")
vix_copy_runtime_libraries(VIX_INSTALLER_RUNTIME_LIBRARIES "${VIX_BOOTSTRAP_OUTPUT_ROOT}" "installer")
vix_copy_runtime_license_files("${VIX_BOOTSTRAP_OUTPUT_ROOT}")
configure_file("${VIX_RELEASE_TRUST}" "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/vix/trust/releases.toml" COPYONLY)
configure_file("${VIX_DISTRIBUTION_SOURCE}" "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/vix/distribution/source.toml" COPYONLY)
file(MAKE_DIRECTORY "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/doc/vix")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/README.md"
               "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/doc/vix/README.md" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../LICENSE"
               "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/doc/vix/LICENSE" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../modules/cli/LICENSE"
               "${VIX_BOOTSTRAP_OUTPUT_ROOT}/share/doc/vix/CLI-LICENSE" COPYONLY)
