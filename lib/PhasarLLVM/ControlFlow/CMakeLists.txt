file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_llvm_pointer
  phasar_llvm_typehierarchy
  phasar_db
  phasar_utils
  phasar_controlflow
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Demangle
)

add_phasar_library(phasar_llvm_controlflow
  ${CONTROLFLOW_SRC}
)

set_target_properties(phasar_llvm_controlflow
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
