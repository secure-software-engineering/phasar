set(Sources
    exceptions_01.cpp
    exceptions_02.cpp
    exceptions_03.cpp
    exceptions_04.cpp
    exceptions_05.cpp
    exceptions_06.cpp
    exceptions_07.cpp
)

foreach(TEST_SRC ${Sources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)


