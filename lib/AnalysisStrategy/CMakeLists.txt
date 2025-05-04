file(GLOB_RECURSE ANALYSIS_STRATEGY_SRC *.h *.cpp)

add_phasar_library(phasar_analysis_strategy
  ${ANALYSIS_STRATEGY_SRC}
  LLVM_LINK_COMPONENTS Support
)
