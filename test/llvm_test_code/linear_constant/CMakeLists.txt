set(lca_files
  basic_01.cpp
  basic_02.cpp
  basic_03.cpp
  basic_04.cpp
  basic_05.cpp
  branch_01.cpp
  branch_02.cpp
  branch_03.cpp
  branch_04.cpp
  branch_05.cpp
  branch_06.cpp
  branch_07.cpp
  call_01.cpp
  call_02.cpp
  call_03.cpp
  call_04.cpp
  call_05.cpp
)

set(lca_files_mem2reg
  basic_06.cpp
)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
