set(VARA_PROBLEM_SOURCES
	IDEVarTabulationProblemTest.cpp
	IFDSVarTabulationProblemTest.cpp
)

foreach(TEST_SRC ${VARA_PROBLEM_SOURCES})
	add_phasar_unittest(${TEST_SRC})
	get_filename_component(test ${TEST_SRC} NAME_WE)
	target_link_libraries(${test}
		z3
	)
endforeach(TEST_SRC)
