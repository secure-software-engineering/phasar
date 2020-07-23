set(ArrayVarSources
  array_01.c
  array_02.c
  # TODO XTC unsupported feature
  # array_03.c
  array_04.c
)

foreach(TEST_SRC ${ArrayVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
