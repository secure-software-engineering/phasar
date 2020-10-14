#file(GLOB variability_files *.c *.cpp)

set(variability_files
  crypt01.c
  crypt02.c
  crypt03.c
  crypt04.c
)

foreach(TEST_SRC ${variability_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  #generate_ll_file(FILE ${TEST_SRC_FILE})
  xtc_making_plans_for_nigell(FILE ${TEST_SRC_FILE} DEBUG)
endforeach(TEST_SRC)
