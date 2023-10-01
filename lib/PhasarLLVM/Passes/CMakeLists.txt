file(GLOB_RECURSE PASSES_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
  Demangle
)

add_phasar_library(phasar_passes
  ${PASSES_SRC}
)

set_target_properties(phasar_passes
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
