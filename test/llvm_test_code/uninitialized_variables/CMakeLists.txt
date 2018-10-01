set(NoMem2RegSources
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
  binop_uninit.cpp
  some_locals.cpp
  uninit.c
)

set(Mem2RegSources
  first_inst_call.cpp
)

foreach(TEST_SRC ${NoMem2RegSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${Mem2RegSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
