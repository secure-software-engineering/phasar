set(BranchingVarSources
  branching_01.c
  branching_02.c
)

foreach(TEST_SRC ${BranchingVarSources})
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
