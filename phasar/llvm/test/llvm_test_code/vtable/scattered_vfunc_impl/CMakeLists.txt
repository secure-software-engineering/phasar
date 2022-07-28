set(NoMem2regSources
  base.cpp
  child_1.cpp
  child_2.cpp
  main.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
