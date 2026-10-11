add_subdirectory(Problems)

set(IfdsIdeSources
  DefaultFlowFunctionTest.cpp
  EdgeFunctionComposerTest.cpp
  EdgeFunctionSingletonCacheTest.cpp
  InteractiveIDESolverTest.cpp
  SparseIDESolverTest.cpp
  IterativeIDESolverTest.cpp
  DFISolverTest.cpp
  CFLFieldSensTest.cpp
  WPDSSolverTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
