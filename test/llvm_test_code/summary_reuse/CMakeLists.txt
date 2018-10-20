set(NoMem2regSources
  summary_reuse_01.cpp
  summary_reuse_02.cpp
  summary_reuse_03.cpp
  summary_reuse_04.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
