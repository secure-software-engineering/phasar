set(NoMem2regSources
  member_access_01.cpp
  member_access_02.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
