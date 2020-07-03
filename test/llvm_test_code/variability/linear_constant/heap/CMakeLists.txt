set(HeapVarSources
  heap_01.c
  heap_02.c
)

foreach(TEST_SRC ${HeapVarSources})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
