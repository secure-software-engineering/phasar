file(GLOB_RECURSE MONO_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_mono
  ${MONO_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  LLVM_LINK_COMPONENTS
    Support

  MODULE_FILES
    Mono.cppm
)
