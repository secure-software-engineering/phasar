set(taint_tests
  taint_01.cpp
  taint_02.cpp
  taint_03.cpp
  taint_04.cpp
  taint_05.cpp
)

set(taint_tests_mem2reg
  taint_01_m2r.cpp
  taint_03_m2r.cpp
)


foreach(TEST_SRC ${taint_tests})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)


foreach(TEST_SRC ${taint_tests_mem2reg})
  generate_ll_file(${TEST_SRC} mem2reg)
endforeach(TEST_SRC)