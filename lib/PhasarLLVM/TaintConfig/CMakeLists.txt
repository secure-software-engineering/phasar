file(GLOB_RECURSE TAINTCONFIG_SRC *.h *.cpp)

add_phasar_library(phasar_taintconfig
  ${TAINTCONFIG_SRC}

  LINKS
    phasar_utils
    phasar_llvm_db
    phasar_llvm_utils
    phasar_controlflow
    phasar_llvm_controlflow

    nlohmann_json_schema_validator

  LLVM_LINK_COMPONENTS
    Core
    Support
)
