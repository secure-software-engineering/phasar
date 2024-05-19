file(GLOB_RECURSE PSR_LLVM_DB_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_db
  ${PSR_LLVM_DB_SRC}

  LINKS
    phasar_utils
    phasar_llvm_utils

  LLVM_LINK_COMPONENTS
    Core
    Support
    IRReader
)
