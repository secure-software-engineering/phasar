set(fields
  array_1.cpp
  array_2.cpp
  array_3.cpp
  base_variable_1.cpp
  field_1.cpp
  field_2.cpp
  field_3.cpp
)

foreach(TEST_SRC ${fields})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
