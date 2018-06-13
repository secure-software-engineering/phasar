set(NoMem2regSources
  basic_01.cpp
  basic_02.cpp
  basic_03.cpp
  basic_04.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
