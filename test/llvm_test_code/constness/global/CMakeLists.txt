set(Mem2regSources
  global_01.cpp
  global_02.cpp
  global_03.cpp
  global_04.cpp
  global_05.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
