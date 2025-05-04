add_subdirectory(Problems)
add_subdirectory(Variability)

set(IfdsIdeSources
  DefaultFlowFunctionTest.cpp
  EdgeFunctionComposerTest.cpp
  EdgeFunctionSingletonCacheTest.cpp
  InteractiveIDESolverTest.cpp
  SparseIDESolverTest.cpp
  IterativeIDESolverTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
