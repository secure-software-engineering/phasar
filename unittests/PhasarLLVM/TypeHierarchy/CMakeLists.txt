set(PointerSources
    DIBasedTypeHierarchySerializationTest.cpp
	DIBasedTypeHierarchyTest.cpp
	LLVMTypeHierarchySerializationTest.cpp
	LLVMTypeHierarchyTest.cpp
)

foreach(TEST_SRC ${PointerSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
