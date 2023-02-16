file(GLOB_RECURSE PSR_LLVM_DB_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_db
  phasar_utils
  phasar_llvm_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  IRReader
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_llvm_db
    SHARED
    ${PSR_LLVM_DB_SRC}
  )
else()
  add_phasar_library(phasar_llvm_db
    STATIC
    ${PSR_LLVM_DB_SRC}
  )
endif()

set_target_properties(phasar_llvm_db
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
