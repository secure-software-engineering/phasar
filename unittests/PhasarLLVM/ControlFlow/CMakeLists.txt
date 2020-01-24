set(ControlFlowSources
	LLVMBasedCFGTest.cpp
	LLVMBasedICFGTest.cpp
	LLVMBasedICFG_CHATest.cpp
	LLVMBasedICFG_DTATest.cpp
	LLVMBasedICFG_OTFTest.cpp
	LLVMBasedICFG_RTATest.cpp
	LLVMBasedBackwardCFGTest.cpp
	LLVMBasedBackwardICFGTest.cpp
)

foreach(TEST_SRC ${ControlFlowSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)

set(VARA_PROBLEM_SOURCES
	LLVMBasedVariabilityCFGTest.cpp
)

foreach(TEST_SRC ${VARA_PROBLEM_SOURCES})
	add_phasar_unittest(${TEST_SRC})
	get_filename_component(test ${TEST_SRC} NAME_WE)
	target_link_libraries(${test}
		z3
	)
endforeach(TEST_SRC)
