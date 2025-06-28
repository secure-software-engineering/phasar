file(GLOB_RECURSE PATHSENSITIVITY_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_pathsensitivity
  ${PATHSENSITIVITY_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  MODULE_FILES
    PathSensitivity.cppm
)
