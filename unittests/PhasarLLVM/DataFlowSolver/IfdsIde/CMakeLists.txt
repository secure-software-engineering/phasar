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

if(UNIX)
  # Append -fno-omit-frame-pointer to get better stack traces.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=thread")
endif()

foreach(TEST_SRC ${ThreadedIfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
