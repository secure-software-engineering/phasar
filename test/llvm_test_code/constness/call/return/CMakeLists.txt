set(Mem2regSources
  call_ret_01.cpp
  call_ret_02.cpp
  call_ret_03.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(${TEST_SRC} mem2reg)
endforeach(TEST_SRC)
