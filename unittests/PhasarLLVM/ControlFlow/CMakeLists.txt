set(ControlFlowSources
	LLVMBasedCFGTest.cpp
	LLVMBasedICFGTest.cpp
	LLVMBasedBackwardCFGTest.cpp
	LLVMBasedBackwardICFGTest.cpp
	CHATest.cpp
	DTATest.cpp
	OTFTest.cpp
	RTATest.cpp

)

foreach(TEST_SRC ${ControlFlowSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
