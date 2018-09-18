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
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

add_subdirectory(dummy_source_sink)
