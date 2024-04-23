set(IfdsIdeProblemSources
	IFDSConstAnalysisTest.cpp
	IFDSTaintAnalysisTest.cpp
	IDEInstInteractionAnalysisTest.cpp
	IDELinearConstantAnalysisTest.cpp
	IDELinearConstantAnalysis_DotTest.cpp
	IFDSUninitializedVariablesTest.cpp
	IDEGeneralizedLCATest.cpp
	IDEExtendedTaintAnalysisTest.cpp
	IDETSAnalysisFileIOTest.cpp
	IDEFeatureInteractionAnalysisTest.cpp
)

if(PHASAR_BUILD_OPENSSL_TS_UNITTESTS)
  list(APPEND IfdsIdeProblemSources
    IDETSAnalysisOpenSSLEVPKDFTest.cpp
    IDETSAnalysisOpenSSLSecureHeapTest.cpp
    IDETSAnalysisOpenSSLSecureMemoryTest.cpp
  )
endif()

if(BUILD_SWIFT_TESTS)
  list(APPEND IfdsIdeProblemSources
	  IDELinearConstantAnalysisSwiftTest.cpp
  )
endif()

test_require_config_file("DOTGraphConfig.json")

foreach(TEST_SRC ${IfdsIdeProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
