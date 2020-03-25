if(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)
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
else()
  set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDEInstInteractionAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IFDSUninitializedVariablesTest.cpp
  )
endif(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)


foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
