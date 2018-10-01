set(NoMem2regSources
  basic_01.cpp
  basic_02.cpp
  basic_03.cpp
  basic_04.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
