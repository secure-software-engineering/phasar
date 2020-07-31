add_subdirectory(Problems)

set(IfdsIdeSources
  EdgeFunctionComposerTest.cpp
)

foreach(TEST_SRC ${IfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)

set(ThreadedIfdsIdeSources
  EdgeFunctionSingletonFactoryTest.cpp
)

if(LLVM_ON_UNIX)
  append_common_sanitizer_flags()
  append("-fsanitize=thread" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
endif()

foreach(TEST_SRC ${ThreadedIfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
