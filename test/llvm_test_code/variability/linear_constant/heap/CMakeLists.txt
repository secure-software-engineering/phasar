set(HeapVarSources
  # TODO XTC unsupported feature
  # heap_01.c
  # TODO XTC unsupported feature
  # heap_02.c
  # TODO XTC unsupported feature
  # heap_03.c
)

foreach(TEST_SRC ${HeapVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
