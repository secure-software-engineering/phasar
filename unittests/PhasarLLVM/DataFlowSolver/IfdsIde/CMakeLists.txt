add_subdirectory(Problems)
add_subdirectory(Variability)

set(IfdsIdeSources
	EdgeFunctionComposerTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
