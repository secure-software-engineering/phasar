add_subdirectory(Problems)

set(IfdsIdeSources
  DefaultFlowFunctionsTest.cpp
  EdgeFunctionComposerTest.cpp
  EdgeFunctionSingletonCacheTest.cpp
  InteractiveIDESolverTest.cpp
  SparseIDESolverTest.cpp
  IterativeIDESolverTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
