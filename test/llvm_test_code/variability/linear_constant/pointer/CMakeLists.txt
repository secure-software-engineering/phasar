set(PointerVarSources
  pointer_01.c
  # TODO XTC unsupported feature
  # pointer_02.c
  # TODO XTC unsupported feature
  # pointer_03.c
)

foreach(TEST_SRC ${PointerVarSources})
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
