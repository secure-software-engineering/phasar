set(NoMem2regSources
  type_hierarchy_1.cpp
  type_hierarchy_2.cpp
  type_hierarchy_3.cpp
  type_hierarchy_4.cpp
  type_hierarchy_5.cpp
  type_hierarchy_6.cpp
  type_hierarchy_7.cpp
  type_hierarchy_8.cpp
  type_hierarchy_9.cpp
  type_hierarchy_10.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(${TEST_SRC})
endforeach(TEST_SRC)
