file(GLOB_RECURSE MONOIFDS_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_monoifds
  ${MONOIFDS_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  LLVM_LINK_COMPONENTS
    Support

  MODULE_FILES
    MonoIfds.cppm
)
