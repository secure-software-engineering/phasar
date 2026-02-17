set(lca_files
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  call_01.cpp
  dynamic_01.cpp
  global_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
  context_01.c
  context_02.c
  context_03.c
  context_04_0.c
  context_04_1.c
  context_05_0.c
  context_05_1.c
  context_06_0.c
  context_06_1.c
  context_07.c
  context_08.c
  context_09_0.c
  context_09_1.c
  context_10_0.c
  context_10_1.c
  context_11_0.c
  context_11_1.c
  context_12_0.c
  context_12_1.c
  context_13_0.c
  context_13_1.c
  context_14_0.c
  context_14_1.c
  context_14_2.c
  indirection_01.c
  indirection_02.c
  indirection_03.c
  indirection_04.c
  indirection_05.c
  indirection_06.c
  indirection_07.c
  indirection_08.cpp
  indirection_09.cpp
  indirection_10.cpp
)

set(lca_files_mem2reg
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
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
