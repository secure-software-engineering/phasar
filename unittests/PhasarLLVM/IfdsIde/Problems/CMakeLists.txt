set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IDETypeStateAnalysisTest.cpp
)

foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
