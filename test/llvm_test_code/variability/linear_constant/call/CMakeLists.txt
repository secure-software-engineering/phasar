set(CallVarSources
  # TODO XTC unsupported feature
  # call_01.c
  # TODO XTC unsupported feature
  # call_02.c
  call_03.c
  # TODO XTC unsupported feature
  # call_04.c
  call_05.c
)
foreach(TEST_SRC ${CallVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
