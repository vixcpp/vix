# Vendored dependencies for restricted environments.
#
# Keep legacy providers callable instead of creating them at include time: the
# SQLite bundled path includes this file, but must not activate unrelated deps.
function(vix_add_vendored_spdlog)
  if (EXISTS "${CMAKE_SOURCE_DIR}/third_party/spdlog/include/spdlog/spdlog.h")
    if (NOT TARGET spdlog::spdlog_header_only)
      add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
      set_target_properties(spdlog::spdlog_header_only PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/spdlog/include"
      )
      message(STATUS "[deps] Using vendored spdlog header-only (third_party/spdlog)")
    endif()
  endif()
endfunction()

function(vix_add_vendored_zlib)
  if (TARGET vix_zlib)
    return()
  endif()

  set(_vix_zlib_source_dir "${CMAKE_SOURCE_DIR}/third_party/zlib")
  set(_vix_zlib_sources
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
  )

  foreach(_vix_zlib_file IN LISTS _vix_zlib_sources)
    if (NOT EXISTS "${_vix_zlib_source_dir}/${_vix_zlib_file}")
      message(FATAL_ERROR
        "Vendored zlib source is missing ${_vix_zlib_file}: ${_vix_zlib_source_dir}"
      )
    endif()
  endforeach()
  foreach(_vix_zlib_header IN ITEMS zlib.h zconf.h)
    if (NOT EXISTS "${_vix_zlib_source_dir}/${_vix_zlib_header}")
      message(FATAL_ERROR
        "Vendored zlib header is missing ${_vix_zlib_header}: ${_vix_zlib_source_dir}"
      )
    endif()
  endforeach()

  list(TRANSFORM _vix_zlib_sources PREPEND "${_vix_zlib_source_dir}/")
  add_library(vix_zlib STATIC ${_vix_zlib_sources})
  add_library(vix::zlib ALIAS vix_zlib)
  set_target_properties(vix_zlib PROPERTIES EXPORT_NAME zlib)

  target_compile_definitions(vix_zlib PRIVATE ZLIB_BUILD)
  target_include_directories(vix_zlib PUBLIC
    $<BUILD_INTERFACE:${_vix_zlib_source_dir}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  if (VIX_ENABLE_INSTALL)
    install(TARGETS vix_zlib
      EXPORT VixTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(FILES
      "${_vix_zlib_source_dir}/zlib.h"
      "${_vix_zlib_source_dir}/zconf.h"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  message(STATUS "[deps] Using bundled zlib sources (third_party/zlib)")
endfunction()

function(vix_add_vendored_brotli)
  if (TARGET vix_brotli)
    return()
  endif()

  set(_vix_brotli_source_dir "${CMAKE_SOURCE_DIR}/third_party/brotli")
  set(_vix_brotli_include_dir "${_vix_brotli_source_dir}/c/include")
  set(_vix_brotli_sources
    c/common/constants.c
    c/common/context.c
    c/common/dictionary.c
    c/common/platform.c
    c/common/shared_dictionary.c
    c/common/transform.c
    c/enc/backward_references.c
    c/enc/backward_references_hq.c
    c/enc/bit_cost.c
    c/enc/block_splitter.c
    c/enc/brotli_bit_stream.c
    c/enc/cluster.c
    c/enc/command.c
    c/enc/compound_dictionary.c
    c/enc/compress_fragment.c
    c/enc/compress_fragment_two_pass.c
    c/enc/dictionary_hash.c
    c/enc/encode.c
    c/enc/encoder_dict.c
    c/enc/entropy_encode.c
    c/enc/fast_log.c
    c/enc/histogram.c
    c/enc/literal_cost.c
    c/enc/memory.c
    c/enc/metablock.c
    c/enc/static_dict.c
    c/enc/static_dict_lut.c
    c/enc/static_init.c
    c/enc/utf8_util.c
  )
  set(_vix_brotli_headers
    brotli/encode.h
    brotli/port.h
    brotli/shared_dictionary.h
    brotli/types.h
  )

  foreach(_vix_brotli_file IN LISTS _vix_brotli_sources)
    if (NOT EXISTS "${_vix_brotli_source_dir}/${_vix_brotli_file}")
      message(FATAL_ERROR
        "Vendored Brotli encoder source is missing ${_vix_brotli_file}: ${_vix_brotli_source_dir}"
      )
    endif()
  endforeach()
  foreach(_vix_brotli_header IN LISTS _vix_brotli_headers)
    if (NOT EXISTS "${_vix_brotli_include_dir}/${_vix_brotli_header}")
      message(FATAL_ERROR
        "Vendored Brotli public header is missing ${_vix_brotli_header}: ${_vix_brotli_include_dir}"
      )
    endif()
  endforeach()

  list(TRANSFORM _vix_brotli_sources PREPEND "${_vix_brotli_source_dir}/")
  add_library(vix_brotli STATIC ${_vix_brotli_sources})
  add_library(vix::brotli ALIAS vix_brotli)
  set_target_properties(vix_brotli PROPERTIES EXPORT_NAME brotli)

  target_include_directories(vix_brotli PUBLIC
    $<BUILD_INTERFACE:${_vix_brotli_include_dir}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  if (UNIX)
    target_link_libraries(vix_brotli PUBLIC m)
  endif()

  if (VIX_ENABLE_INSTALL)
    install(TARGETS vix_brotli
      EXPORT VixTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(FILES
      "${_vix_brotli_include_dir}/brotli/encode.h"
      "${_vix_brotli_include_dir}/brotli/port.h"
      "${_vix_brotli_include_dir}/brotli/shared_dictionary.h"
      "${_vix_brotli_include_dir}/brotli/types.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/brotli"
    )
  endif()

  message(STATUS "[deps] Using bundled Brotli encoder sources (third_party/brotli)")
endfunction()

function(vix_add_vendored_sqlite3)
  if (TARGET vix_sqlite3)
    return()
  endif()

  set(_vix_sqlite_amalgamation_dir
    "${CMAKE_SOURCE_DIR}/third_party/sqlite/amalgamation"
  )
  foreach(_vix_sqlite_file IN ITEMS sqlite3.c sqlite3.h sqlite3ext.h)
    if (NOT EXISTS "${_vix_sqlite_amalgamation_dir}/${_vix_sqlite_file}")
      message(FATAL_ERROR
        "Vendored SQLite amalgamation is missing ${_vix_sqlite_file}: "
        "${_vix_sqlite_amalgamation_dir}"
      )
    endif()
  endforeach()

  add_library(vix_sqlite3 STATIC
    "${_vix_sqlite_amalgamation_dir}/sqlite3.c"
  )
  add_library(vix::sqlite3 ALIAS vix_sqlite3)
  set_target_properties(vix_sqlite3 PROPERTIES EXPORT_NAME sqlite3)

  target_compile_definitions(vix_sqlite3 PRIVATE SQLITE_THREADSAFE=1)
  target_include_directories(vix_sqlite3 PUBLIC
    $<BUILD_INTERFACE:${_vix_sqlite_amalgamation_dir}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  if (VIX_ENABLE_INSTALL)
    install(TARGETS vix_sqlite3
      EXPORT VixTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(FILES
      "${_vix_sqlite_amalgamation_dir}/sqlite3.h"
      "${_vix_sqlite_amalgamation_dir}/sqlite3ext.h"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  message(STATUS "[deps] Using bundled SQLite3 amalgamation (third_party/sqlite)")
endfunction()
