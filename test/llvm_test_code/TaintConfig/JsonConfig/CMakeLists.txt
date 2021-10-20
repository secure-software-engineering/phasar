set(JsonTaintConfigTestSources
  array_01.c
  array_02.c
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  data_member_01.cpp
  fun_member_01.cpp
  fun_member_02.cpp
  name_mangling_01.cpp
  static_fun_01.cpp
  static_fun_02.cpp
)

foreach(TEST_SRC ${JsonTaintConfigTestSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

set(JsonTaintConfigFiles
  array_01_config.json
  array_02_config.json
  basic_01_config.json
  basic_02_config.json
  basic_03_config.json
  basic_04_config.json
  data_member_01_config.json
  fun_member_01_config.json
  fun_member_02_config.json
  name_mangling_01_config.json
  static_fun_01_config.json
  static_fun_02_config.json
)

foreach(TEST_CONFIG ${JsonTaintConfigFiles})
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_CONFIG} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach(TEST_CONFIG)

