set(NoMem2regSources
  base.cpp
  derived.cpp
  main.cpp
  utils.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
