set(NoMem2regSources
  cmd_args.c
  fread.c
  growing_example.cpp
  if_else.cpp
  operator_shift.cpp
  printf.c
  print.cpp
  read.c
  source_sink_function_test.c
  taint_1.cpp
  taint_2.cpp
  taint_3.cpp
  taint_4.cpp
  taint_5.cpp
  taint_6.cpp
  taint_7.cpp
  taint_8.cpp
  taint_9.c
  taint_10.c
  taint_11.c
  taint_12.c
  taint_13.c
  taint_2_v2.cpp
  taint_2_v2_1.cpp
  taint_4_v2.cpp
  taint_14.cpp
  taint_14_1.cpp
  taint_15.cpp
  taint_15_1.cpp
  virtual_calls.cpp
  virtual_calls_v2.cpp
  struct_member.cpp
  dynamic_memory.cpp
  dynamic_memory_simple.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

add_subdirectory(dummy_source_sink)
