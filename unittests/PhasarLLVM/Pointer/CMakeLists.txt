set(ControlFlowSources
	LLVMPointsToSetTest.cpp
)

foreach(TEST_SRC ${ControlFlowSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
