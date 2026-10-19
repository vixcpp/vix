foreach(required VIX_TARGET_SDK_PREFIX VIX_TARGET_SDK_VERSION VIX_TARGET_SDK_TARGET VIX_TARGET_SDK_TOOLCHAIN)
  if (NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()
if (NOT IS_DIRECTORY "${VIX_TARGET_SDK_PREFIX}/include" OR
    NOT EXISTS "${VIX_TARGET_SDK_PREFIX}/lib/cmake/Vix/VixConfig.cmake")
  message(FATAL_ERROR "VIX_TARGET_SDK_PREFIX is not an installed Vix SDK")
endif()
file(MAKE_DIRECTORY "${VIX_TARGET_SDK_PREFIX}/share/vix")
file(WRITE "${VIX_TARGET_SDK_PREFIX}/share/vix/target-sdk.toml"
  "format = 1\nversion = \"${VIX_TARGET_SDK_VERSION}\"\ntarget = \"${VIX_TARGET_SDK_TARGET}\"\ntoolchain = \"${VIX_TARGET_SDK_TOOLCHAIN}\"\n")
