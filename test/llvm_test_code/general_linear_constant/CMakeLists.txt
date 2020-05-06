set(Sources
    BranchTest.c
    FloatDivision.c
    FPtest.c
    GlobalVariableTest.c
    Imprecision.c
    length.c
    NullTest.c
    ReturnConstTest.c
    SimpleFunctionTest.c
    SimpleTest.c
    StringBranchTest.c
    StringTest.c
    StringTest.cpp
)

foreach(TEST_SRC ${Sources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
