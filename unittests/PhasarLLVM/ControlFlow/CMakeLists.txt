set(ControlFlowSources
	LLVMBasedCFGTest.cpp
	LLVMBasedICFGTest.cpp
	LLVMBasedICFG_CHATest.cpp
	LLVMBasedICFG_DTATest.cpp
	LLVMBasedICFG_OTFTest.cpp
	LLVMBasedICFG_RTATest.cpp
)

foreach(TEST_SRC ${ControlFlowSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
