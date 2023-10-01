file(GLOB_RECURSE DB_SRC *.h *.cpp)

include_directories(
  ${SQLITE3_INCLUDE_DIR}
)

set(PHASAR_LINK_LIBS
  phasar_passes
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Support
)

  add_phasar_library(phasar_db
    ${DB_SRC}
  )

target_link_libraries(phasar_db
  LINK_PRIVATE
  ${SQLITE3_LIBRARY}
)

set_target_properties(phasar_db
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
