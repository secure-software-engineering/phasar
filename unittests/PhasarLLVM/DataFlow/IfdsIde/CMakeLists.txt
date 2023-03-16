add_subdirectory(Problems)

set(IfdsIdeSources
  EdgeFunctionComposerTest.cpp
  EdgeFunctionSingletonCacheTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
