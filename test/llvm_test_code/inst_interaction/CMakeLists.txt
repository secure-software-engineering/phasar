set(DbgSources
  basic_01.cpp
)

foreach(TEST_SRC ${DbgSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
