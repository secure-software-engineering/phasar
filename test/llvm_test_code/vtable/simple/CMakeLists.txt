set(NoMem2regSources
  base.cpp
  main.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
