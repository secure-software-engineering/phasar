set(Mem2regSources
  stl_array_01.cpp
  stl_array_02.cpp
  stl_array_03.cpp
  stl_array_04.cpp
  stl_array_05.cpp
  stl_array_06.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
