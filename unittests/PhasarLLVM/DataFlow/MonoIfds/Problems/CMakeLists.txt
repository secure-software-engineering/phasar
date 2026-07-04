file(GLOB_RECURSE MONOIFDS_TEST_SRC *.cpp)

foreach(TEST_SRC ${MONOIFDS_TEST_SRC})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
