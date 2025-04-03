set(NoMem2regSources
  all_flow_functions_01.cpp
  int_call_no_params_01.cpp
  int_call_one_param_01.cpp
  int_call_pointer_param_01.cpp
  void_call_no_params_01.cpp
  void_call_one_param_01.cpp
  void_call_pointer_param_01.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach(subdir)
