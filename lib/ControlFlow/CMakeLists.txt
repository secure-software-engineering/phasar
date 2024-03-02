file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

add_phasar_library(phasar_controlflow
  ${CONTROLFLOW_SRC}
  LLVM_LINK_COMPONENTS Support
  LINK_PRIVATE nlohmann_json::nlohmann_json
)
