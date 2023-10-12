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

set_target_properties(phasar_passes
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
