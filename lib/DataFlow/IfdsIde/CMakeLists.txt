file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_ifdside
  ${IFDSIDE_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  MODULE_FILES
    IfdsIde.cppm
)
