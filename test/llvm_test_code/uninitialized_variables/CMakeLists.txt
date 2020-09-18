set(NoMem2RegSources
  all_uninit.cpp
  callnoret.c
  calltoret.c
  ctor.cpp
  ctor_default.cpp
  dyn_mem.cpp
  exception.cpp
  while_uninit_1.cpp
  first_inst_call.cpp
  global_variable.cpp
  growing_example.cpp
  multiple_calls.cpp
  reassing_uninit.cpp
  binop_uninit.cpp
  some_locals.cpp
  uninit.c
  pointer_1.cpp
  main_args.cpp
  main_args_init.cpp
  simple_store.cpp
  callsite.cpp
  return_uninit.cpp
  struct_member_init.cpp
  struct_member_uninit.cpp
  struct_member_uninit2.cpp
  struct_test.cpp
  sanitizer.cpp
  sanitizer2.cpp
  sanitizer_uninit.cpp
  array_init.cpp
  array_init_simple.cpp
  recursion.cpp
  virtual_call.cpp
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
