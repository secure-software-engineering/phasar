set(Mem2regSources
  call_param_01.cpp
  call_param_02.cpp
  call_param_03.cpp
  call_param_04.cpp
  call_param_05.cpp
  call_param_06.cpp
  call_param_07.cpp
  call_param_08.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(${TEST_SRC} mem2reg)
endforeach(TEST_SRC)
