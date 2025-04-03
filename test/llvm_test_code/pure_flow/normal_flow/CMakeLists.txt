set(NoMem2regSources
  normal_flow_01.cpp
  normal_flow_02.cpp
  normal_flow_03.cpp
  normal_flow_04.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
