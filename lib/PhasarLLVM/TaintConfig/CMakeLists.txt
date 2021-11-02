file(GLOB_RECURSE TAINTCONFIG_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_db
  phasar_phasarllvm_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_taintconfig
    SHARED
    ${TAINTCONFIG_SRC}
  )
else()
  add_phasar_library(phasar_taintconfig
    STATIC
    ${TAINTCONFIG_SRC}
  )
endif()

target_link_libraries(phasar_taintconfig
  LINK_PUBLIC
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator
)

set_target_properties(phasar_taintconfig
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
