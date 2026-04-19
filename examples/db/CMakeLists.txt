option(VIX_DB_BUILD_EXAMPLES "Build Vix DB examples" OFF)

if (VIX_DB_BUILD_EXAMPLES)
  function(vix_db_example name)
    add_executable(vix_db_example_${name} ${name}.cpp)
    target_link_libraries(vix_db_example_${name}
      PRIVATE
        vix::db
    )
    target_compile_features(vix_db_example_${name} PRIVATE cxx_std_20)
  endfunction()

  vix_db_example(basic_connect)
  vix_db_example(prepared_query)
  vix_db_example(transaction)
  vix_db_example(migrations)
  vix_db_example(sqlite_basic)
  vix_db_example(pool_usage)

  if (VIX_DB_HAS_MYSQL)
    vix_db_example(mysql_basic)
  endif()
endif()
