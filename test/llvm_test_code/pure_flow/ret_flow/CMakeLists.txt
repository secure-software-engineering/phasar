set(NoMem2regSources
  ret_flow_01.cpp
  ret_flow_02.cpp
  ret_flow_03.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
