# Domain-local entry point for the pinned WASI Preview 2 toolchain.
# Install official wasi-sdk 34.0 and set VIX_WASI_SDK_ROOT to its directory.
if ((NOT DEFINED VIX_WASI_SDK_ROOT OR VIX_WASI_SDK_ROOT STREQUAL "") AND
    DEFINED ENV{VIX_WASI_SDK_ROOT} AND NOT "$ENV{VIX_WASI_SDK_ROOT}" STREQUAL "")
  set(VIX_WASI_SDK_ROOT "$ENV{VIX_WASI_SDK_ROOT}")
endif()
if (NOT DEFINED VIX_WASI_SDK_ROOT OR VIX_WASI_SDK_ROOT STREQUAL "")
  message(FATAL_ERROR "Set VIX_WASI_SDK_ROOT to the wasi-sdk 34.0 installation directory.")
endif()

get_filename_component(_vix_wasi_sdk_root "${VIX_WASI_SDK_ROOT}" REALPATH)
set(_vix_wasi_p2_toolchain "${_vix_wasi_sdk_root}/share/cmake/wasi-sdk-p2.cmake")
if (NOT EXISTS "${_vix_wasi_p2_toolchain}")
  message(FATAL_ERROR
    "${_vix_wasi_p2_toolchain} is missing; Vix's WASI probe requires the official wasi-sdk 34.0 P2 toolchain.")
endif()

include("${_vix_wasi_p2_toolchain}")
set(VIX_WASI_TARGET "wasm32-wasip2" CACHE INTERNAL "Vix Network WASI target")
