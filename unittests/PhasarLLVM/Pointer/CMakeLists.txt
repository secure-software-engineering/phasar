set(PointerFlowSources
	LLVMAliasSetTest.cpp
	LLVMAliasSetSerializationTest.cpp
	FilteredLLVMAliasSetTest.cpp
)

if (PHASAR_USE_SVF)
	list(APPEND PointerFlowSources SVFAliasSetTest.cpp)
endif()

foreach(TEST_SRC ${PointerFlowSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)

if(PHASAR_USE_SVF)
	target_link_libraries(SVFAliasSetTest PRIVATE phasar_llvm_pointer_svf)
endif()
