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

add_phasar_library(phasar_llvm_db
  ${PSR_LLVM_DB_SRC}
)

set_target_properties(phasar_llvm_db
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
