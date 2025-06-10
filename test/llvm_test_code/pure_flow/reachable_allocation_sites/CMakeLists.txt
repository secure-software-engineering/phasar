set(NoMem2regSources
  reachable_allocation_sites_01.cpp
  reachable_allocation_sites_02.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
