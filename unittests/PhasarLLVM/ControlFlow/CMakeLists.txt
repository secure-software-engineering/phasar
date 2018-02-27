set(ControlFlowSources
	LLVMBasedCFGTest.cpp
)

add_llvm_unittest(ControlFlowTests
	${ControlFlowSources}
)
