set(lca_files
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  basic_05.c
  basic_06.cpp
  basic_07.cpp
  call_01.cpp
  dynamic_01.cpp
  global_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
  context_01.c
  context_02.c
  context_03.c
  context_04.c
  context_05.c
  context_06.c
  context_07.c
  context_08.c
  context_09.c
  context_10.c
  context_11.c
  context_12.c
  context_13.c
  context_14.cpp
)

set(lca_files_mem2reg
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  basic_05.c
  basic_06.cpp
  basic_07.cpp
  dynamic_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
