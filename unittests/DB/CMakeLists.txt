if(SQLite3_FOUND)
	set(DBSources
		HexastoreTest.cpp
	)

	foreach(TEST_SRC ${DBSources})
		add_phasar_unittest(${TEST_SRC})
	endforeach(TEST_SRC)
endif()
