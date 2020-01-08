set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDEInstInteractionAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IDETypeStateAnalysisFileIOTest.cpp
	IDETypeStateAnalysisOpenSSLTest.cpp
	IFDSUninitializedVariablesTest.cpp
)

foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
