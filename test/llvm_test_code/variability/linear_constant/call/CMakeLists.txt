set(CallVarSources
  # TODO XTC unsupported feature
  # call_01.c
  # TODO XTC unsupported feature
  # call_02.c
  # TODO XTC unsupported feature
  # call_03.c
  # TODO XTC unsupported feature
  # call_04.c
  # TODO XTC unsupported feature
  # call_05.c
  # TODO XTC unsupported feature
  # call_06.c
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
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
