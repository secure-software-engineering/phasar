file(GLOB_RECURSE SOLVER_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_ifdside_solver
  ${SOLVER_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  MODULE_FILES
    dataflow_ifdside_solver.cppm
)
