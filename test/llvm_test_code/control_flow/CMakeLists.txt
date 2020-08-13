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

set(DbgSources
  ignore_dbg_insts_1.cpp
  ignore_dbg_insts_2.cpp
  ignore_dbg_insts_3.cpp
  ignore_dbg_insts_4.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_DBG_SRC ${DbgSources})
  generate_ll_file(FILE ${TEST_DBG_SRC} DEBUG)
endforeach(TEST_DBG_SRC)
