set(NoMem2regSources
  int_call_no_params_01.cpp
  int_call_one_param_01.cpp
  void_call_no_params_01.cpp
  void_call_one_param_01.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
