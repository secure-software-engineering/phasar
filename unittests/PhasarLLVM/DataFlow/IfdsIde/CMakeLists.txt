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

if(UNIX AND CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
  # Only add thread sanitizer for the test if asan is disabled because both
  # cannot be active at the same time.
  if (NOT "${LLVM_USE_SANITIZER}" MATCHES ".*Address.*" AND NOT "${CMAKE_CXX_FLAGS}" MATCHES ".*address.*")
    # Append -fno-omit-frame-pointer to get better stack traces.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=thread")
  endif()
endif()

foreach(TEST_SRC ${ThreadedIfdsIdeSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
