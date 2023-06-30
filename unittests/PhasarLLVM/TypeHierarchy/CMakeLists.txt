set(PointerSources
	DIBasedTypeHierarchyTest.cpp
	TypeGraphTest.cpp
)

foreach(TEST_SRC ${PointerSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
