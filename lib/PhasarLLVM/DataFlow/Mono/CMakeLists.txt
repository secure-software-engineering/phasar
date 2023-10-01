file(GLOB_RECURSE MONO_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_llvm_utils
  phasar_llvm_controlflow
  phasar_db
  phasar_taintconfig
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

add_phasar_library(phasar_mono
  ${MONO_SRC}
)

set_target_properties(phasar_mono
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
