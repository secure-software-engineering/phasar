file(GLOB lca_files *.c *.cpp)

set(lca_files_mem2reg_dbg
  basic_06.cpp
)

set(lca_files_mem2reg
  call_12.cpp
  call_13.cpp
)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE})
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg_dbg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG)
endforeach(TEST_SRC)
