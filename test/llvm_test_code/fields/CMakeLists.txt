set(fields
  array_1.cpp
  base_variable_1.cpp
	field_1.cpp
)

foreach(TEST_SRC ${fields})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
