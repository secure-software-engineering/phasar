file(GLOB TEST_TYPES_SRC *.c *.cpp)

foreach(TEST_SRC ${TEST_TYPES_SRC})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE} DEBUG)
endforeach(TEST_SRC)
