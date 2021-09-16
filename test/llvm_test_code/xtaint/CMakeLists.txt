set(XTAINT_SOURCES
    xtaint01.cpp
    # xtaint01_json.cpp
    xtaint02.cpp
    xtaint03.cpp
    xtaint04.cpp
    xtaint05.cpp
    xtaint06.cpp
#     xtaint07.cpp
#     xtaint08.cpp
    xtaint09.cpp
    xtaint09_1.cpp
    xtaint10.cpp
    xtaint11.cpp
    xtaint12.cpp
    xtaint13.cpp
    xtaint14.cpp
    xtaint15.cpp
    xtaint16.cpp
    xtaint17.cpp
    xtaint18.cpp
    xtaint19.cpp
    xtaint20.cpp
    xtaint21.cpp
)

set(XTAINT_DBG_SOURCES
    xtaint01_json.cpp
)

foreach(TEST_SRC ${XTAINT_SOURCES})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${XTAINT_DBG_SOURCES})
 generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)