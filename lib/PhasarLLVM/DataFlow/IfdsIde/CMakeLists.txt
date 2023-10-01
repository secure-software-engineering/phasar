file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_llvm_pointer
  phasar_llvm
  phasar_llvm_typehierarchy
  phasar_llvm_controlflow
  phasar_llvm_utils
  phasar_db
  phasar_taintconfig
  ${Boost_LIBRARIES}
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Demangle
)

add_phasar_library(phasar_llvm_ifdside
  ${IFDSIDE_SRC}
)

set_target_properties(phasar_llvm_ifdside
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
