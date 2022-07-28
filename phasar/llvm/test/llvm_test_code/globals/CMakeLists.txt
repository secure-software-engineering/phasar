set(NoMem2regSources
  globals_1.cpp
  globals_2.cpp
  globals_ctor_1.cpp
  globals_ctor_2_1.cpp
  globals_ctor_2_2.cpp
  globals_dtor_1.cpp
  globals_lca_1.cpp
  globals_lca_2.cpp
  globals_lca_3.cpp
  globals_lca_4.cpp
  globals_lca_4_1.cpp
  globals_lca_5.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
