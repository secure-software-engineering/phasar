set(basics
  module.cpp
  seven_structs.cpp
  two_structs.cpp
)

foreach(TEST_SRC ${basics})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
