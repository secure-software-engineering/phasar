set(NoMem2regSources
  malloc_free.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
