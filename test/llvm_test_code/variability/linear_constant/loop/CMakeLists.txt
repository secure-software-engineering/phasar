set(LoopVarSources
  loop_01.c
  loop_02.c
  loop_03.c
  # TODO XTC unsupported feature
  # loop_04.c
)

foreach(TEST_SRC ${LoopVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
