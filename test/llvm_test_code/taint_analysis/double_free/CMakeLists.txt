set(NoMem2regSources
  df_01.c
  df_02.c
  df_03.c
  df_04_1.c
  df_04.c
  df_05.c
  df_06.c
  df_07_1.c
  df_07.c
  df_08.c
  df_09.c
  df_10.c
  df_11.c
  df_12.c
  df_13.c
  df_14.c
  df_15.c
  df_16.c
  df_17.c
  df_18.c
  df_19.c
  df_20.c
  df_21.c
  df_22.c
  df_ctx_01.c
  df_ctx_02.c
  df_ctx_03.c
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
