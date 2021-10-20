set(TestTextFiles
  test.txt
)

foreach(TEST_FILE ${TestTextFiles})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach(TEST_FILE)
