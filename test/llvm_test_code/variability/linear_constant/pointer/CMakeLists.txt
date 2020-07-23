set(PointerVarSources
  pointer_01.c
  # TODO XTC unsupported feature
  # pointer_02.c
  # TODO XTC unsupported feature
  # pointer_03.c
)

foreach(TEST_SRC ${PointerVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
