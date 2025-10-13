set(PointerSources
  DIBasedTypeHierarchySerializationTest.cpp
	DIBasedTypeHierarchyTest.cpp
	LLVMTypeHierarchySerializationTest.cpp
	LLVMTypeHierarchyTest.cpp
)

foreach(TEST_SRC ${PointerSources})
	add_phasar_unittest(${TEST_SRC})
	run_test_for_prowraw(${TEST_SRC})
endforeach(TEST_SRC)
