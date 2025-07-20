#file(GLOB variability_files *.c *.cpp)

set(variability_files
  hash01.c
  hash02.c
  hash03.c
  hash04.c
  #TODO: XTC unsupported feature
  #hash05.c
)

foreach(TEST_SRC ${variability_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC_FILE})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
