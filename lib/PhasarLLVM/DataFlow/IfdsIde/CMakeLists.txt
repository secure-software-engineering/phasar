file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_ifdside
  ${IFDSIDE_SRC}

  LINKS
    phasar_config
    phasar_utils
    phasar_llvm_pointer
    phasar_llvm
    phasar_llvm_typehierarchy
    phasar_llvm_controlflow
    phasar_llvm_utils
    phasar_taintconfig

  LLVM_LINK_COMPONENTS
    Core
    Support
    Demangle

  LINK_PRIVATE
    ${Boost_LIBRARIES}
)
