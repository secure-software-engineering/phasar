set(UtilsSources
  BitVectorSetTest.cpp
  EquivalenceClassMapTest.cpp
  IOTest.cpp
  LLVMIRToSrcTest.cpp
  LLVMShorthandsTest.cpp
  PAMMTest.cpp
  StableVectorTest.cpp
  AnalysisPrinterTest.cpp
)

if(PHASAR_ENABLE_DYNAMIC_LOG)
  list(APPEND UtilsSources LoggerTest.cpp)
endif()

foreach(TEST_SRC ${UtilsSources})
  add_phasar_unittest(${TEST_SRC})
endforeach(TEST_SRC)
