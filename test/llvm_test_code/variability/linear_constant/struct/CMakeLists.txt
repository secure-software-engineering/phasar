# TODO XTC unsupported feature
#
#set(StructVarSources
#  struct_01.c
#  struct_02.c
#  struct_03.c
#  struct_04.c
#)
#
#foreach(TEST_SRC ${StructVarSources})
#  xtc_making_plans_for_nigell(TEST_SRC_DESUGARED FILE ${TEST_SRC})
#  generate_ll_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_DESUGARED} DEBUG)
#endforeach(TEST_SRC)
