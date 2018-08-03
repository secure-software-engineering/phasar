set(NoMem2regSources
  globals_1.cpp
  globals_2.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
