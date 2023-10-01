file(GLOB_RECURSE TAINTCONFIG_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_db
  phasar_llvm_db
  phasar_llvm_utils
  phasar_controlflow
  phasar_llvm_controlflow
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

add_phasar_library(phasar_taintconfig
  ${TAINTCONFIG_SRC}
)

target_link_libraries(phasar_taintconfig
  LINK_PRIVATE
  nlohmann_json_schema_validator
)

set_target_properties(phasar_taintconfig
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
