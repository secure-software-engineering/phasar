set(ArrayVarSources
  array_01.c
  array_02.c
  # TODO XTC unsupported feature
  # array_03.c
  # array_04.c
)

foreach(TEST_SRC ${ArrayVarSources})
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
