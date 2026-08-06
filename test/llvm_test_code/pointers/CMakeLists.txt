set(lca_files
  andersen_otf_interproc.c
  andersen_otf_fp.c
  andersen_otf_global_init.c
  andersen_otf_merge_load.c
  andersen_otf_fp_already_processed.c
  andersen_otf_extern_callback.c
  andersen_otf_fp_struct_field.c
  andersen_otf_vtable.cpp
  andersen_otf_vtable2.cpp
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  call_01.cpp
  dynamic_01.cpp
  factory_01.c
  factory_02.c
  andersen_otf_fnptr_table_basic.c
  andersen_otf_fnptr_table_two_sites.c
  andersen_otf_fnptr_table_dynamic_index.c
  andersen_otf_fnptr_table_indirect_value.c
  andersen_otf_fnptr_table_memcpy.c
  andersen_otf_bug_a4_aggregate_ret.c
  andersen_otf_bug_a4_atomics.c
  global_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
  context_01.c
  context_02.c
  context_03.c
  context_04_0.c
  context_04_1.c
  context_05_0.c
  context_05_1.c
  context_06_0.c
  context_06_1.c
  context_07.c
  context_08.c
  context_09_0.c
  context_09_1.c
  context_10_0.c
  context_10_1.c
  context_11_0.c
  context_11_1.c
  context_12_0.c
  context_12_1.c
  context_13_0.c
  context_13_1.c
  context_14_0.c
  context_14_1.c
  context_14_2.c
  context_15.c
  context_16.c
  context_17.c
  indirection_01.c
  indirection_02.c
  indirection_03.c
  indirection_04.c
  indirection_05.c
  indirection_06.c
  indirection_07.c
  indirection_08.cpp
  indirection_09.cpp
  indirection_10.cpp
  memssa_kill_01.c
  memssa_branch_01.c
)

set(lca_files_mem2reg
  andersen_otf_interproc.c
  andersen_otf_fp.c
  andersen_otf_libc.c
  andersen_otf_struct_vtable.c
  andersen_otf_bug_a1_poison_scc.c
  andersen_otf_bug_a2_loop_gep.c
  andersen_otf_bug_a3_stranded_pending.c
  basic_01.c
  basic_02.c
  basic_03.c
  basic_04.c
  dynamic_01.cpp
  inter_dynamic_01.cpp
  inter_dynamic_02.cpp
)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files_mem2reg})
  generate_ll_file(FILE ${TEST_SRC} MEM2REG DEBUG)
endforeach(TEST_SRC)
