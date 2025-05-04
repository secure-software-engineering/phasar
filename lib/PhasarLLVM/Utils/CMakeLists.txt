file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_utils
  ${UTILS_SRC}

  LINKS
    phasar_config
    phasar_utils

  LLVM_LINK_COMPONENTS
    Core
    Support
    BitWriter
    Demangle
)
