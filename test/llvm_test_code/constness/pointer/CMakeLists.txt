set(NoMem2regSources
  pointer_01.cpp
  pointer_02.cpp
  pointer_03.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

generate_ll_file(FILE pointer_04.cpp MEM2REG DEBUG)
