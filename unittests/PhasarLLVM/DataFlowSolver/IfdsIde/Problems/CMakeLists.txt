if(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)
  set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDEInstInteractionAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IDELinearConstantAnalysis_DotTest.cpp
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
	IDELinearConstantAnalysis_DotTest.cpp
	IFDSUninitializedVariablesTest.cpp
  )
endif(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)

test_require_config_file("DOTGraphConfig.json")

foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
