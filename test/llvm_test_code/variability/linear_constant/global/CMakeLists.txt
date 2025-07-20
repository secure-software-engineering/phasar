set(GlobalVarSources
  global_01.c
  # TODO XTC unsupported feature
  # global_02.c
  # TODO XTC unsupported feature
  # global_03.c
  # TODO XTC unsupported feature
  # global_04.c
  global_05.c
)

foreach(TEST_SRC ${GlobalVarSources})
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
