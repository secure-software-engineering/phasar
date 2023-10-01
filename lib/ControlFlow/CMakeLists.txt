file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
)

set(LLVM_LINK_COMPONENTS
  Support
)

add_phasar_library(phasar_controlflow
  ${CONTROLFLOW_SRC}
)

set_target_properties(phasar_controlflow
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
