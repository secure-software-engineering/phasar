set(NoMem2regSources
	function_object_1.cpp
	function_pointer_1.c
	function_pointer_2.cpp
	function_pointer_3.cpp
	special_member_functions_1.cpp
	static_callsite_10.cpp
	static_callsite_11.cpp
	static_callsite_12.cpp
	static_callsite_13.cpp
	static_callsite_1.c
	static_callsite_2.c
	static_callsite_3.c
	static_callsite_4.cpp
	static_callsite_5.cpp
	static_callsite_6.cpp
	static_callsite_7.cpp
	static_callsite_8.cpp
	static_callsite_9.cpp
	type_graph_1.cpp
	virtual_call_1.cpp
	virtual_call_2.cpp
	virtual_call_3.cpp
	virtual_call_4.cpp
	virtual_call_5.cpp
	virtual_call_6.cpp
	virtual_call_7.cpp
	virtual_call_8.cpp
	virtual_call_9.cpp
	global_ctor_dtor_1.cpp
	global_ctor_dtor_2.cpp
	global_ctor_dtor_3.cpp
	global_ctor_dtor_4.cpp
)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC})
endforeach(TEST_SRC)

foreach(TEST_SRC ${NoMem2regSources})
  generate_ll_file(FILE ${TEST_SRC} DEBUG)
endforeach(TEST_SRC)
