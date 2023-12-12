
if(NOT "${SQLITE3_LIBRARY}" STREQUAL "SQLITE3_LIBRARY-NOTFOUND")
  set(DBSources
  	HexastoreTest.cpp
  )  
  foreach(TEST_SRC ${DBSources})
  	add_phasar_unittest(${TEST_SRC})
  endforeach(TEST_SRC)
endif()
