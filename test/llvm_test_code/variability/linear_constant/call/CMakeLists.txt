set(CallVarSources
  # TODO XTC unsupported feature
  # call_01.c
  # TODO XTC unsupported feature
  # call_02.c
  call_03.c
  # TODO XTC unsupported feature
  # call_04.c
  call_05.c
  call_06.c
  call_07.c
  # TODO XTC unsupported feature
  # call_08.c
  # TODO XTC unsupported feature
  # call_09.c
  # TODO XTC unsupported feature
  # call_10.c
  call_11.c
  call_12.c
  # TODO XTC unsupported feature
  # call_13.c
  # TODO XTC unsupported feature
  # call_14.c
  # TODO XTC unsupported feature
  # call_15.c
  # TODO XTC unsupported feature
  # call_16.c
)
foreach(TEST_SRC ${CallVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
