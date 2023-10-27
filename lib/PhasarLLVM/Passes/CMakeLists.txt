file(GLOB_RECURSE PASSES_SRC *.h *.cpp)

add_phasar_library(phasar_passes
  ${PASSES_SRC}

  LINKS
    phasar_utils

  LLVM_LINK_COMPONENTS
    Core
    Support
    Analysis
    Demangle
)
