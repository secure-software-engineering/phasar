file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  BitWriter
  Demangle
)

# Handle the library files
add_phasar_library(phasar_llvm_utils
  ${UTILS_SRC}
)

set_target_properties(phasar_llvm_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
