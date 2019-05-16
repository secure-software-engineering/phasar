file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

# Compile phasar_pass once into an object
add_library(phasar_pass_o OBJECT
  ${PHASARPASS_SRC}
)

add_phasar_library(phasar_pass
  SHARED
  $<TARGET_OBJECTS:phasar_pass_o>
)

target_link_libraries(phasar_pass
  boost_filesystem
  boost_graph
  boost_log
  boost_program_options
  boost_system

  phasar_config
  phasar_controlflow
  phasar_db
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_phasarllvm_utils
  phasar_pointer
  phasar_utils_without
)

# phasar_pass_without can be used to link phasar without LLVMDeps
add_phasar_library(phasar_pass_without
  $<TARGET_OBJECTS:phasar_pass_o>
)

target_link_libraries(phasar_pass_without
  boost_filesystem
  boost_graph
  boost_log
  boost_program_options
  boost_system

  phasar_config
  phasar_controlflow
  phasar_db_without
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_phasarllvm_utils
  phasar_pointer
  phasar_utils_without
)

set_target_properties(phasar_pass
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
