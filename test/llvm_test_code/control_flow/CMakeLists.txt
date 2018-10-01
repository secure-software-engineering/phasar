set(NoMem2regSources
  branch.cpp 
  calls.cpp 
  function_call.cpp
  function_call_2.cpp
  global_stmt.cpp 
  if_else.cpp
  loop.cpp
  multi_calls.cpp
  simple_call.cpp
  switch.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
