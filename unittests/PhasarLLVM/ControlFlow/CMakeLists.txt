set(ControlFlowSources
  LLVMBasedCFGTest.cpp
  LLVMBasedICFGTest.cpp
  LLVMBasedICFG_CHATest.cpp
  LLVMBasedICFG_OTFTest.cpp
  LLVMBasedICFG_RTATest.cpp
  LLVMBasedICFG_RTA_MultipleInheritanceTest.cpp
  LLVMBasedBackwardCFGTest.cpp
  LLVMBasedBackwardICFGTest.cpp
  LLVMBasedICFGExportTest.cpp
  LLVMBasedICFGGlobCtorDtorTest.cpp
  LLVMBasedICFGSerializationTest.cpp
  LLVMVFTableProviderTest.cpp
  VTACallGraphTest.cpp
)

set(LLVM_LINK_COMPONENTS Linker) # The CtorDtorTest needs the linker
foreach(TEST_SRC ${ControlFlowSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
