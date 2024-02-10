if(PHASAR_HAS_SQLITE3)
  file(GLOB_RECURSE DB_SRC *.h *.cpp)
else()
  set(DB_SRC DB.cpp)
endif()

add_phasar_library(phasar_db
  ${DB_SRC}
  LINKS phasar_passes phasar_utils
  LLVM_LINK_COMPONENTS Support
)

if(PHASAR_HAS_SQLITE3)
  target_link_libraries(phasar_db
    PRIVATE ${SQLITE3_LIBRARY}
  )
  target_include_directories(phasar_db
    PRIVATE ${SQLITE3_INCLUDE_DIR}
  )
endif()
