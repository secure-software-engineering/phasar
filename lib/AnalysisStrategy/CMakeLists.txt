file(GLOB_RECURSE ANALYSIS_STRATEGY_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_db
  phasar_controlflow
)

set(LLVM_LINK_COMPONENTS
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_analysis_strategy
    SHARED
    ${ANALYSIS_STRATEGY_SRC}
  )
else()
  add_phasar_library(phasar_analysis_strategy
    STATIC
    ${ANALYSIS_STRATEGY_SRC}
  )
endif()

set_target_properties(phasar_analysis_strategy
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
