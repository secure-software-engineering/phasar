set(MonoSources
	DistInterMonoSolverTest.cpp
	InterMonoFullConstantPropagationTest.cpp
	InterMonoTaintAnalysisTest.cpp
	IntraMonoUninitVariablesTest.cpp
	IntraMonoFullConstantPropagationTest.cpp
)

foreach(TEST_SRC ${MonoSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
