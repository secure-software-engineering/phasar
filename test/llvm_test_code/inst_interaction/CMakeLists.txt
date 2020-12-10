set(Sources
  basic_01.cpp
  basic_02.cpp
  basic_03.cpp
  basic_04.cpp
  basic_05.cpp
  basic_06.cpp
  basic_07.cpp
  basic_08.cpp
  basic_09.cpp
  basic_10.cpp
  call_01.cpp
  call_02.cpp
  call_03.cpp
  call_04.cpp
  call_05.cpp
  call_06.cpp
  global_01.cpp
  heap_01.cpp
  KillTest.cpp
  struct_01.cpp
)

foreach(TEST_SRC ${Sources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
