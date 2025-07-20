set(LoopVarSources
  loop_01.c
  loop_02.c
  loop_03.c
  # TODO XTC unsupported feature
  # loop_04.c
)

foreach(TEST_SRC ${LoopVarSources})
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
