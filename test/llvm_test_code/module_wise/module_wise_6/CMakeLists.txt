set(NoMem2regSources
  main.cpp
  src1.cpp
  src2.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
