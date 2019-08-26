set(UtilsSources
	TaintConfigurationTest.cpp
)

foreach(TEST_SRC ${UtilsSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
