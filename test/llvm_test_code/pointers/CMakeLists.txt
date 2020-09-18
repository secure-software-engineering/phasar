set(lca_files
  basic_01.cpp
  call_01.cpp
  dynamic_01.cpp
  global_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
)

set(lca_files_mem2reg
  basic_01.cpp
  dynamic_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
