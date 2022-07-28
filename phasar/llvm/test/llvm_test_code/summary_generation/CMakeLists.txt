set(NoMem2regSources
  summary_1.cpp
  summary_2.cpp
  summary_3.cpp
  summary_4.cpp
  summary_5.cpp
  summary_6.cpp
  summary_class_1.cpp
  summary_class_2.cpp
  summary_class_3.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
