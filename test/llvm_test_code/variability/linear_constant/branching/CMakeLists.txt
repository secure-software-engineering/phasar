set(BranchingVarSources
  branching_01.c
  branching_02.c
)

foreach(TEST_SRC ${BranchingVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
