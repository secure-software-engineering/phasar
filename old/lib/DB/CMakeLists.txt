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
  Core
  # Vectorize
  # ScalarOpts
  # Instrumentation
  # ObjCARCOpts
  # TransformUtils
  # CodeGen
  # Analysis
  # ipo
  IRReader
  # InstCombine
  Linker
  BitWriter
  Passes
  Demangle
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_db
    SHARED
    ${DB_SRC}
  )
else()
  add_phasar_library(phasar_db
    STATIC
    ${DB_SRC}
  )
endif()

target_link_libraries(phasar_db
  LINK_PUBLIC
  ${SQLITE3_LIBRARY}
)

set_target_properties(phasar_db
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
