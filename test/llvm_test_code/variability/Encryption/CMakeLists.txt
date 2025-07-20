#file(GLOB variability_files *.c *.cpp)

set(variability_files
  crypt01.c
  crypt02.c
  crypt03.c
  crypt04.c
)

foreach(TEST_SRC ${variability_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC_FILE})
  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
endforeach(TEST_SRC)
