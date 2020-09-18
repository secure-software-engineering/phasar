set(WPDSProblemSources
	WPDSLinearConstantAnalysisTest.cpp
)

foreach(TEST_SRC ${WPDSProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
