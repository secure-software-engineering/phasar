add_subdirectory(ControlFlow)
add_subdirectory(DataFlow)
add_subdirectory(DB)
add_subdirectory(Domain)
add_subdirectory(Passes)
add_subdirectory(Pointer)
add_subdirectory(TaintConfig)
add_subdirectory(TypeHierarchy)
add_subdirectory(Utils)

file(GLOB PHASAR_LLVM_SRC *.h *.cpp)

add_phasar_library(phasar_llvm
  ${PHASAR_LLVM_SRC}

  LINKS
    phasar_llvm_controlflow
    phasar_llvm_dataflow
    phasar_llvm_db
    phasar_llvm_domain
    phasar_llvm_ifdside
    phasar_llvm_pointer
    phasar_llvm_typehierarchy
    phasar_llvm_utils
    phasar_passes
    phasar_utils
    phasar_taintconfig

  LLVM_LINK_COMPONENTS
    Core
    Support

  MODULE_FILES
    PhasarLLVM.cppm
)
