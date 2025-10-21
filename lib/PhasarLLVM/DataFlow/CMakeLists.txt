add_subdirectory(IfdsIde)
add_subdirectory(Mono)
add_subdirectory(PathSensitivity)

add_phasar_library(phasar_llvm_dataflow
  LINKS
    phasar_llvm_ifdside
    phasar_llvm_mono
    phasar_llvm_pathsensitivity

  MODULE_FILES
    DataFlow.cppm
)
