set(NoMem2regSources
  all_uninit.cpp
  callnoret.c
  calltoret.c
  ctor.cpp
  ctor_default.cpp
  dyn_mem.cpp
  exception.cpp
  first_inst_call.cpp
  global_variable.cpp
  growing_example.cpp
  multiple_calls.cpp
  reassing_uninit.cpp
  some_locals.cpp
  uninit.c
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
