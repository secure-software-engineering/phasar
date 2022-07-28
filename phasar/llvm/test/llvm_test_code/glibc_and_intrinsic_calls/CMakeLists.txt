set(NoMem2regSources
  glibc_and_intrinsics_1.cpp
  glibc_and_intrinsics_2.cpp
  glibc_and_intrinsics_3.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
