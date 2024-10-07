add_subdirectory(cross_module)

set(NoMem2regSources
  callsite.cpp
  callsite_staticmemory.cpp
  interproc_callsite.cpp
  callsite_dynmemory.cpp
  circular_dependencies.cpp
  interproc_callsite_double.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
