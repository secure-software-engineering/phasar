file(GLOB_RECURSE MONO_SRC *.h *.cpp)

add_phasar_library(phasar_dataflow_mono
  ${MONO_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  MODULE_FILES
    Mono.cppm
)
