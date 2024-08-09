file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

add_phasar_library(phasar_controller
  FILES
    ${CONTROLLER_SRC}

  LINKS
    phasar_llvm_ifdside
    phasar_mono
    phasar_llvm_db
    phasar_llvm_pointer
    phasar_llvm_typehierarchy
    phasar_llvm_controlflow
    phasar_llvm_utils
    phasar_utils
    phasar_analysis_strategy
    phasar_taintconfig
    phasar_passes

  LLVM_LINK_COMPONENTS
    Core
    Support
    Demangle

  LINK_PRIVATE
    ${PHASAR_STD_FILESYSTEM}
)
