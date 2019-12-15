set(VaraProblemSources
	IDEVariabilityTabulationProblemTest.cpp
)

foreach(TEST_SRC ${VaraProblemSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)