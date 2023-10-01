file(GLOB_RECURSE ANALYSIS_STRATEGY_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_db
  phasar_controlflow
)

set(LLVM_LINK_COMPONENTS
  Support
)

add_phasar_library(phasar_analysis_strategy
  ${ANALYSIS_STRATEGY_SRC}
)

set_target_properties(phasar_analysis_strategy
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
