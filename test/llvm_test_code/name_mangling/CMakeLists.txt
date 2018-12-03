set(name_mangling
  special_members_1.cpp
  special_members_2.cpp
  special_members_3.cpp
  special_members_4.cpp
)

foreach(TEST_SRC ${name_mangling})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
