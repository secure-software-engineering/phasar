set(VaraProblemSources
	IDEVariabilityTabulationProblemTest.cpp
)

foreach(TEST_SRC ${VaraProblemSources})
	add_phasar_unittest(${TEST_SRC})
	get_filename_component(test ${TEST_SRC} NAME_WE)
	target_link_libraries(${test}
		z3
	)
endforeach(TEST_SRC)