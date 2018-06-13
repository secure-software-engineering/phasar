set(basics
  seven_structs.cpp
  two_structs.cpp
)

foreach(TEST_SRC ${basics})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
