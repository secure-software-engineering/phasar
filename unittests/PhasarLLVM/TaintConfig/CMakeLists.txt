set(TaintConfigTestSources
  TaintConfigTest.cpp
)

foreach(TEST_SRC ${TaintConfigTestSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
