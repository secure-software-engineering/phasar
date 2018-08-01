set(linear_constant
  basic_01.cpp
  basic_02.cpp
  basic_03.cpp
  basic_04.cpp
  basic_05.cpp
  branch_01.cpp
  branch_02.cpp
  call_01.cpp
  call_02.cpp
  call_03.cpp
  call_04.cpp
  call_05.cpp
)
foreach(TEST_SRC ${linear_constant})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)

generate_ll_file(basic_06.cpp mem2reg)
