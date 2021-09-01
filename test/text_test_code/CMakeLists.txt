set(TestTextFiles
  test.txt
)

foreach(TEST_FILE ${TestTextFiles})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach(TEST_FILE)
