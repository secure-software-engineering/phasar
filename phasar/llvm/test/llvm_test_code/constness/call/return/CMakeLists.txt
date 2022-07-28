set(Mem2regSources
  call_ret_01.cpp
  call_ret_02.cpp
  call_ret_03.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
