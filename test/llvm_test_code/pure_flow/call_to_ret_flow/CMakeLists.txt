set(NoMem2regSources
  call_to_ret_flow_01.cpp
  call_to_ret_flow_02.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
