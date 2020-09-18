set(WPDSSources
	WPDSSolverTest.cpp
)

foreach(TEST_SRC ${WPDSSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
