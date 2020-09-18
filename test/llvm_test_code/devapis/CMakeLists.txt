set(taintapiconfig
  source_annotations_01.cpp
)

foreach(TEST_SRC ${taintapiconfig})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
