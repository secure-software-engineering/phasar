set(PATH_TRACING_FILES
  inter_01.cpp
  inter_02.cpp
  inter_03.cpp
  inter_04.cpp
  inter_05.cpp
  inter_06.cpp
  inter_07.cpp
  inter_08.cpp
  inter_09.cpp
  inter_10.cpp
  inter_11.cpp
  inter_12.cpp
  intra_01.cpp
  intra_02.cpp
  intra_03.cpp
  intra_04.cpp
  intra_05.cpp
  intra_06.cpp
  intra_07.cpp
  intra_08.cpp
  intra_09.cpp
  other_01.cpp
)

foreach(TEST_SRC ${PATH_TRACING_FILES})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${PATH_TRACING_FILES})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
