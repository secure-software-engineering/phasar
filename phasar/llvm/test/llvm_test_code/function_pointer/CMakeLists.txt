set(NoMem2regSources
  fptr_1.cpp
  function_ptr.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
