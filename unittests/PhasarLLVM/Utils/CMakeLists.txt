set(UtilsSources
  LatticeDomainTest.cpp
)

test_require_config_file("phasar-source-sink-function.json")

foreach(TEST_SRC ${UtilsSources})
	add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
