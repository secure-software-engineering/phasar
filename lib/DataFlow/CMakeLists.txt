add_subdirectory(IfdsIde)
add_subdirectory(Mono)
add_subdirectory(PathSensitivity)
add_subdirectory(WPDS)

file(GLOB_RECURSE DATAFLOW_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow
  ${DATAFLOW_SRC}

  LINKS
    phasar_dataflow_ifdside
    phasar_dataflow_mono
    phasar_dataflow_pathsensitivity
    phasar_dataflow_wpds

  MODULE_FILES
    DataFlow.cppm
)
