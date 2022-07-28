file(GLOB FullConstantFiles *.c *.cpp)

foreach(TEST_SRC ${FullConstantFiles})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE})
endforeach(TEST_SRC)
