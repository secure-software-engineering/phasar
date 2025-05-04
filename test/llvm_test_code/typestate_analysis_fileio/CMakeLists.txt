file(GLOB typestate_files *.c *.cpp)

foreach(TEST_SRC ${typestate_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE})
endforeach(TEST_SRC)

foreach(TEST_SRC ${typestate_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE} DEBUG)
endforeach(TEST_SRC)
