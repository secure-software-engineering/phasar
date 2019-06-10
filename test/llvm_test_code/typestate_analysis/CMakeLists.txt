set(NoMem2regSources
	typestate_01.c
	typestate_02.c
	typestate_03.c
	typestate_04.c
	typestate_05.c
	typestate_06.c
	typestate_07.c
	typestate_08.c
	typestate_09.c
	typestate_10.c
	typestate_11.c
	typestate_12.c
	typestate_13.c
	typestate_14.c
	typestate_15.c
	typestate_16.c
	typestate_17.c
	typestate_18.c
	typestate_19.c
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)
	