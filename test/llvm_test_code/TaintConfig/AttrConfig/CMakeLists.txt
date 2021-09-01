set(AttrTaintConfigTestSources
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

foreach(TEST_SRC ${AttrTaintConfigTestSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
