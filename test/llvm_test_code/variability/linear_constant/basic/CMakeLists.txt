set(BasicVarSources
  basic_01.c
  basic_02.c
)

foreach(TEST_SRC ${BasicVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
