set(taint_tests
  taint_01.cpp
  taint_02.cpp
  taint_03.cpp
  taint_04.cpp
  taint_05.cpp
  taint_exception_01.cpp
  taint_exception_02.cpp
  taint_exception_03.cpp
  taint_exception_04.cpp
  taint_exception_05.cpp
  taint_exception_06.cpp
  taint_exception_07.cpp
  taint_exception_08.cpp
  taint_exception_09.cpp
  taint_exception_10.cpp
  taint_lib_sum_01.cpp
)

set(taint_tests_mem2reg
  taint_01.cpp
  taint_06.cpp
  taint_exception_01.cpp
)


foreach(TEST_SRC ${taint_tests})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)


foreach(TEST_SRC ${taint_tests_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
