set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDEInstInteractionAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IDETSAnalysisFileIOTest.cpp
	IDETSAnalysisOpenSSLEVPKDFTest.cpp
	IDETSAnalysisOpenSSLSecureHeapTest.cpp
	IDETSAnalysisOpenSSLSecureMemoryTest.cpp
	IFDSUninitializedVariablesTest.cpp
)

foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
