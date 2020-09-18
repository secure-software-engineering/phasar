set(TaintAPISource
	TaintConfigTest.cpp
)

foreach(TEST_SRC ${TaintAPISource})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
