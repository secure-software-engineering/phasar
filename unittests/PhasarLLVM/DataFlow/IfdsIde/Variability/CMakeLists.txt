set(VARA_PROBLEM_SOURCES
	IDEVarTabulationProblemTest.cpp
	IFDSVarTabulationProblemTest.cpp
	IDEVarTAOpenSSLSecureMemoryTest.cpp
	IDEVarTAOpenSSLMDTest.cpp
)

foreach(TEST_SRC ${VARA_PROBLEM_SOURCES})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
