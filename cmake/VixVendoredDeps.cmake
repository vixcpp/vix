# Vendored deps for restricted environments (no git, no system packages)
message(STATUS "[deps] Loading VixVendoredDeps.cmake from: ${CMAKE_CURRENT_LIST_FILE}")


# spdlog (header-only) — as IMPORTED INTERFACE so it doesn't need export
if (EXISTS "${CMAKE_SOURCE_DIR}/third_party/spdlog/include/spdlog/spdlog.h")
  if (NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
    set_target_properties(spdlog::spdlog_header_only PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/spdlog/include"
    )
    message(STATUS "[deps] Using vendored spdlog header-only (third_party/spdlog)")
  endif()
endif()

# SQLite3 (vendored amalgamation) — OBJECT lib (no export)
if (EXISTS "${CMAKE_SOURCE_DIR}/third_party/sqlite/amalgamation/sqlite3.c")
  if (NOT TARGET vix_sqlite3_obj)
    add_library(vix_sqlite3_obj OBJECT
      "${CMAKE_SOURCE_DIR}/third_party/sqlite/amalgamation/sqlite3.c"
    )
    set_target_properties(vix_sqlite3_obj PROPERTIES
      LINKER_LANGUAGE C
    )
    target_include_directories(vix_sqlite3_obj PUBLIC
      "${CMAKE_SOURCE_DIR}/third_party/sqlite/amalgamation"
    )
    message(STATUS "[deps] Using vendored SQLite3 amalgamation (third_party/sqlite)")
  endif()
endif()

