add_subdirectory(cstring)
add_subdirectory(stl_array)

set(Mem2regSources
  array_01.cpp
  array_02.cpp
  array_03.cpp
  array_04.cpp
  array_05.cpp
  array_06.cpp
  array_07.cpp
  array_08.cpp
  array_09.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
