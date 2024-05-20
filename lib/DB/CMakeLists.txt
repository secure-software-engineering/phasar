if(SQLite3_FOUND)
  file(GLOB_RECURSE DB_SRC *.h *.cpp)

  add_phasar_library(phasar_db
    ${DB_SRC}
    LINKS phasar_passes phasar_utils
    LLVM_LINK_COMPONENTS Support
    LINK_PRIVATE SQLite::SQLite3
  )
endif()
