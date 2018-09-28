set(Mem2regSources
  cf_for_01.cpp
  cf_for_02.cpp
  cf_if_01.cpp
  cf_if_02.cpp
  cf_while_01.cpp
)

foreach(TEST_SRC ${Mem2regSources})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
