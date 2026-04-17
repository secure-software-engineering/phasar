file(GLOB_RECURSE WPDS_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_wpds
  ${WPDS_SRC}

  MODULE_FILES
    WPDS.cppm
)
