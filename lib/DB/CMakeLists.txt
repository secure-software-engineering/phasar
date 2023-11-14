file(GLOB_RECURSE DB_SRC *.h *.cpp)

add_phasar_library(phasar_db
  ${DB_SRC}
  LINKS phasar_passes phasar_utils
  LLVM_LINK_COMPONENTS Support
  LINK_PRIVATE ${SQLITE3_LIBRARY}
)

target_include_directories(phasar_db
  PRIVATE ${SQLITE3_INCLUDE_DIR}
)
