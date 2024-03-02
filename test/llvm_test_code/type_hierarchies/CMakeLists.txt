set(NoMem2RegSources
  type_hierarchy_1.cpp
  type_hierarchy_2.cpp
  type_hierarchy_3.cpp
  type_hierarchy_4.cpp
  type_hierarchy_5.cpp
  type_hierarchy_6.cpp
  type_hierarchy_7.cpp
  type_hierarchy_7_b.cpp
  type_hierarchy_8.cpp
  type_hierarchy_9.cpp
  type_hierarchy_10.cpp
  type_hierarchy_11.cpp
  type_hierarchy_12.cpp
  type_hierarchy_12_b.cpp
  type_hierarchy_12_c.cpp
  type_hierarchy_13.cpp
  type_hierarchy_14.cpp
  type_hierarchy_15.cpp
  type_hierarchy_16.cpp
  type_hierarchy_17.cpp
  type_hierarchy_18.cpp
  type_hierarchy_19.cpp
  type_hierarchy_20.cpp
  type_hierarchy_21.cpp
)
foreach(TEST_SRC ${NoMem2RegSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${NoMem2RegSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
