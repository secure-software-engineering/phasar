set(Mem2regSources
  global_01.cpp
  global_02.cpp
  global_03.cpp
  global_04.cpp
  global_05.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(${TEST_SRC} mem2reg)
endforeach(TEST_SRC)
