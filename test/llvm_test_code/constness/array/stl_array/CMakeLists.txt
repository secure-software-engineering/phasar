set(Mem2regSources
  stl_array_01.cpp
  stl_array_02.cpp
  stl_array_03.cpp
  stl_array_04.cpp
  stl_array_05.cpp
  stl_array_06.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(${TEST_SRC} mem2reg)
endforeach(TEST_SRC)
