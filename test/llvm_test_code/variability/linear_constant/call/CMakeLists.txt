set(CallVarSources
  call_01.c
  call_02.c
  call_03.c
)

#foreach(TEST_SRC ${CallVarSources})
#  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
#endforeach(TEST_SRC)
